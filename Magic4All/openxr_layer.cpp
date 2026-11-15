// MIT License
//
// Copyright(c) 2026 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "pch.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>

#include "log.h"
#include "util.h"
#include "gpu_timer.h"
#include "openxr_layer.h"

// OpenXR functions that we call but do not intercept.
// clang-format off
#define OPENXR_LAYER_NEXT_FUNCTIONS(_)      \
    _(xrEnumerateSwapchainImages)           \
    _(xrGetSystem)                          \
    _(xrGetSystemProperties)                \
    _(xrStringToPath)                       \
    _(xrCreateActionSet)                    \
    _(xrCreateAction)                       \
    _(xrCreateActionSpace)                  \
    _(xrCreateReferenceSpace)               \
    _(xrGetActionStatePose)                 \
    _(xrLocateSpace)
// clang-format on

// OpenXR functions that we intercept.
// clang-format off
#define OPENXR_LAYER_HOOKED_FUNCTIONS(_)    \
    _(xrDestroyInstance)                    \
    _(xrPollEvent)                          \
    _(xrCreateSession)                      \
    _(xrDestroySession)                     \
    _(xrBeginSession)                       \
    _(xrSuggestInteractionProfileBindings)  \
    _(xrAttachSessionActionSets)            \
    _(xrSyncActions)                        \
    _(xrWaitFrame)                          \
    _(xrBeginFrame)                         \
    _(xrEndFrame)
// clang-format on

namespace {

    using namespace logging;
    using namespace util;
    using namespace openxr_layer;

#ifdef _WIN64
    constexpr wchar_t k_LibMagicName[] = L"LibMagicD3D1164.dll";
#else
    constexpr wchar_t k_LibMagicName[] = L"LibMagicD3D1132.dll";
#endif

    // Internal LibMagic callback to register swapchain images.
    constexpr uint32_t k_MinImagesPerEye = 5;
    using PFN_submitEyeTextures = void (*)(ID3D11Texture2D** textures,
                                           int count,
                                           const pvrViewPort viewports[2],
                                           const pvrFovPort fovs[2],
                                           bool originBottomLeft);

    // IPC from DFR-UI -> API layer. Mainly used to gate injection.
    constexpr wchar_t k_DfrUiMutexName[] = L"Local\\PimaxMagic4All-DFR-UI";
    constexpr ULONGLONG k_DfrUiCheckIntervalMs = 1000;

    // IPC from API layer -> DFR-UI. Used to communicate current running application.
    constexpr wchar_t k_ApplicationInfoMappingName[] = L"Local\\PimaxMagic4All-OpenXR-App";
    constexpr uint32_t k_ApplicationInfoVersion = 1;
    struct SharedApplicationInfo {
        uint32_t version;
        uint32_t processId; // Written last, 0 while not valid.
        char applicationName[XR_MAX_APPLICATION_NAME_SIZE];

        // Cumulative statistics: DFR-UI averages over its refresh interval from the difference between two reads.
        // Each sum is updated before its sample count, and the 64-bit sums are naturally aligned for atomic access.
        uint64_t cpuFrameTimeSumUs; // Between xrBeginFrame() and xrEndFrame().
        uint64_t gpuFrameTimeSumUs;
        uint32_t cpuFrameTimeSamples;
        uint32_t gpuFrameTimeSamples;
        uint32_t framesSubmitted;
        uint32_t reserved;
    };
    static_assert(offsetof(SharedApplicationInfo, cpuFrameTimeSumUs) == 8 + XR_MAX_APPLICATION_NAME_SIZE);
    static_assert(sizeof(SharedApplicationInfo) == 8 + XR_MAX_APPLICATION_NAME_SIZE + 32);

    constexpr uint32_t k_NumGpuTimers = 3;
    constexpr uint64_t k_NoActionsFrameThreshold = 100;

    bool IsDfrUiRunning() {
        wil::unique_handle mutex;
        *mutex.put() = OpenMutexW(SYNCHRONIZE, FALSE, k_DfrUiMutexName);
        return !!mutex;
    }

    pvrFovPort ToPvrFovPort(const XrFovf& fov) {
        pvrFovPort fovPort{};
        fovPort.UpTan = std::tan(fov.angleUp);
        fovPort.DownTan = std::tan(-fov.angleDown);
        fovPort.LeftTan = std::tan(-fov.angleLeft);
        fovPort.RightTan = std::tan(fov.angleRight);
        return fovPort;
    }

    // Trampolines from the OpenXR entry points to the singleton (defined at the bottom of the file).
    XrResult XRAPI_CALL hook_xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function);
    XrResult XRAPI_CALL hook_xrCreateApiLayerInstance(const XrInstanceCreateInfo* instanceCreateInfo,
                                                      const XrApiLayerCreateInfo* apiLayerInfo,
                                                      XrInstance* instance);
    XrResult XRAPI_CALL hook_xrDestroyInstance(XrInstance instance);
    XrResult XRAPI_CALL hook_xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData);
    XrResult XRAPI_CALL hook_xrCreateSession(XrInstance instance,
                                             const XrSessionCreateInfo* createInfo,
                                             XrSession* session);
    XrResult XRAPI_CALL hook_xrDestroySession(XrSession session);
    XrResult XRAPI_CALL hook_xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo);
    XrResult XRAPI_CALL
    hook_xrSuggestInteractionProfileBindings(XrInstance instance,
                                             const XrInteractionProfileSuggestedBinding* suggestedBindings);
    XrResult XRAPI_CALL hook_xrAttachSessionActionSets(XrSession session,
                                                       const XrSessionActionSetsAttachInfo* attachInfo);
    XrResult XRAPI_CALL hook_xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo);
    XrResult XRAPI_CALL hook_xrWaitFrame(XrSession session,
                                         const XrFrameWaitInfo* frameWaitInfo,
                                         XrFrameState* frameState);
    XrResult XRAPI_CALL hook_xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo);
    XrResult XRAPI_CALL hook_xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo);

    class OpenXrLayer : public IOpenXrLayer {
      public:
        static OpenXrLayer& Get() {
            static OpenXrLayer instance;
            return instance;
        }

        void setTextureArrayStretch(float stretch) override {
            m_textureArrayStretch = stretch;
        }

        std::optional<GazeTransform> getTextureArrayGazeTransform() const override {
            if (!m_isTextureArray.load()) {
                return {};
            }
            return GazeTransform{m_textureArrayCenterX.load(), m_textureArrayGazeScaleX.load()};
        }

        bool isOpenXrApplication() const override {
            return m_isOpenXrApplication.load();
        }

        bool isEyeGazeSupported() const override {
            return m_isEyeGazeSupported.load();
        }

        std::optional<XrPosef> getEyeGaze() const override {
            std::unique_lock lock(m_eyeGazeMutex);
            return m_eyeGaze;
        }

        XrResult xrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrCreateSession");

            const XrResult result = m_xrCreateSession(instance, createInfo, session);
            if (XR_SUCCEEDED(result) && createInfo) {
                // We can only hook D3D11 sessions.
                const XrBaseInStructure* entry = reinterpret_cast<const XrBaseInStructure*>(createInfo->next);
                while (entry && entry->type != XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) {
                    entry = entry->next;
                }

                std::unique_lock lock(m_mutex);
                if (entry) {
                    m_session = *session;
                    m_d3d11Device = reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(entry)->device;

                    for (uint32_t i = 0; i < k_NumGpuTimers; i++) {
                        m_gpuTimers[i] = D3D11GpuTimer::Create(m_d3d11Device);
                    }
                    m_currentGpuTimer = 0;
                    m_isGpuTimerRunning = false;

                    // Make sure LibMagic starts in OpenXR mode (no OpenVR hooks), however it ends up being loaded.
                    SetEnvironmentVariableW(L"VR_TYPE", L"openxr");

                    Log("Session is D3D11\n");

                    m_isEyeGazeSupported = m_isEyeGazeExtensionEnabled && isEyeGazeInteractionSupported() &&
                                           initializeEyeGazeInteraction(*session);
                    m_needPollEvent = m_needAttachActionSets = m_needSyncActions = true;
                } else {
                    Log("Session is not D3D11, using LibMagic is not possible\n");
                }
                TraceLoggingWriteTagged(local, "xrCreateSession", TLArg(!!entry, "IsD3D11"));
            }

            TraceLoggingWriteStop(local, "xrCreateSession", TLArg(xr::ToCString(result), "Result"));
            return result;
        }

        XrResult xrDestroySession(XrSession session) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrDestroySession");

            const XrResult result = m_xrDestroySession(session);
            if (XR_SUCCEEDED(result)) {
                std::unique_lock lock(m_mutex);
                if (session == m_session) {
                    m_session = XR_NULL_HANDLE;
                    m_d3d11Device = nullptr;
                    for (uint32_t i = 0; i < k_NumGpuTimers; i++) {
                        m_gpuTimers[i].reset();
                    }
                    m_isGpuTimerRunning = false;
                    m_frameBeginTime.reset();
                    m_isStereoSession = false;
                    m_isEyeGazeSupported = false;
                    m_eyeSpace = m_viewSpace = XR_NULL_HANDLE;
                    {
                        std::unique_lock eyeGazeLock(m_eyeGazeMutex);
                        m_eyeGaze.reset();
                    }
                    // XrAction handles belong to the instance, so we don't touch anything here.
                }
            }

            TraceLoggingWriteStop(local, "xrDestroySession", TLArg(xr::ToCString(result), "Result"));
            return result;
        }

        XrResult xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrBeginSession");

            const XrResult result = m_xrBeginSession(session, beginInfo);
            if (XR_SUCCEEDED(result) && beginInfo) {
                std::unique_lock lock(m_mutex);
                if (session == m_session) {
                    m_isStereoSession =
                        beginInfo->primaryViewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    if (!m_isStereoSession) {
                        Log(fmt::format("View configuration {} is not stereo, using LibMagic is not possible\n",
                                        xr::ToCString(beginInfo->primaryViewConfigurationType)));
                    }

                    m_framesElapsed = 0;
                }
                TraceLoggingWriteTagged(
                    local,
                    "xrBeginSession",
                    TLArg(xr::ToCString(beginInfo->primaryViewConfigurationType), "ViewConfiguration"));
            }

            TraceLoggingWriteStop(local, "xrBeginSession", TLArg(xr::ToCString(result), "Result"));
            return result;
        }

        XrResult xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrWaitFrame");

            const XrResult result = m_xrWaitFrame(session, frameWaitInfo, frameState);
            if (XR_SUCCEEDED(result) && frameState) {
                std::unique_lock lock(m_mutex);
                if (isSessionSupported(session) && m_submitEyeTextures) {
                    if (m_isEyeGazeSupported) {
                        // Give the app some frames to tell us what it intends to do regarding the action system.
                        if (++m_framesElapsed > k_NoActionsFrameThreshold) {
                            // Some applications may not advance the instance event state machine (via xrPollEvent()),
                            // which causes actions to always return an inactive state. Force xrPollEvent() here if
                            // needed.
                            if (m_needPollEvent) {
                                XrEventDataBuffer buf{XR_TYPE_EVENT_DATA_BUFFER};
                                m_xrPollEvent(m_instance, &buf);
                            }

                            if (m_needAttachActionSets) {
                                // This will clear the m_needAttachActionSets flag.
                                XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
                                const XrResult attachResult = attachSessionActionSets(session, &attachInfo);
                                if (XR_FAILED(attachResult)) {
                                    Log(fmt::format("Failed to attach the eye tracker action set: {}\n",
                                                    xr::ToCString(attachResult)));
                                }
                            }

                            // If an application does not use motion controllers, it is not calling xrSyncActions().
                            // Make a call here in order to synchronize our action set.
                            if (m_needSyncActions) {
                                XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
                                XrActiveActionSet actionSet{};
                                actionSet.actionSet = m_eyeTrackerActionSet;
                                syncInfo.activeActionSets = &actionSet;
                                syncInfo.countActiveActionSets = 1;
                                m_xrSyncActions(session, &syncInfo);
                            }
                        }

                        // Query the eye tracker ahead of the frame.
                        const std::optional<XrPosef> eyeGaze = getEyeGazeInteraction(frameState->predictedDisplayTime);
                        {
                            std::unique_lock eyeGazeLock(m_eyeGazeMutex);
                            m_eyeGaze = eyeGaze;
                        }
                        TraceLoggingWriteTagged(local, "xrWaitFrame", TLArg(!!eyeGaze, "IsGazeValid"));
                    }
                }
            }

            TraceLoggingWriteStop(local, "xrWaitFrame", TLArg(xr::ToCString(result), "Result"));
            return result;
        }

        XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrBeginFrame");

            const XrResult result = m_xrBeginFrame(session, frameBeginInfo);
            if (XR_SUCCEEDED(result)) {
                std::unique_lock lock(m_mutex);

                if (isSessionSupported(session) && m_submitEyeTextures) {
                    // The application renders between xrBeginFrame() and xrEndFrame(). We do not measure from
                    // xrWaitFrame(): pipelined applications (e.g. Unreal, XrSceneLib) call it from another thread, ahead
                    // of rendering, which would measure their pipeline latency instead. A frame that was never ended
                    // (XR_FRAME_DISCARDED) is simply restarted here.
                    m_frameBeginTime = std::chrono::steady_clock::now();

                    startGpuTimer();
                }
            }

            TraceLoggingWriteStop(local, "xrBeginFrame", TLArg(xr::ToCString(result), "Result"));
            return result;
        }

        XrResult xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrEndFrame");

            const auto now = std::chrono::steady_clock::now();
            std::unique_lock lock(m_mutex);
            if (isSessionSupported(session)) {
                // See xrBeginFrame().
                if (m_frameBeginTime) {
                    const auto cpuFrameTimeUs =
                        std::chrono::duration_cast<std::chrono::microseconds>(now - *m_frameBeginTime).count();
                    publishCpuFrameTime(static_cast<uint64_t>(cpuFrameTimeUs));
                    TraceLoggingWriteTagged(local, "xrEndFrame", TLArg(cpuFrameTimeUs, "AppCpuFrameTimeUs"));
                    m_frameBeginTime.reset();
                }

                if (m_isGpuTimerRunning) {
                    m_gpuTimers[m_currentGpuTimer]->stop();
                    m_isGpuTimerRunning = false;
                    m_currentGpuTimer = (m_currentGpuTimer + 1) % k_NumGpuTimers;
                }

                tryLoadLibMagic();

                // Hand the eye textures of the first projection layer to LibMagic.
                const XrCompositionLayerProjection* projection = nullptr;
                if (m_submitEyeTextures && frameEndInfo && m_xrEnumerateSwapchainImages) {
                    for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                        if (frameEndInfo->layers[i] &&
                            frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
                            projection = reinterpret_cast<const XrCompositionLayerProjection*>(frameEndInfo->layers[i]);
                            break;
                        }
                    }
                }

                if (projection && projection->viewCount >= xr::StereoView::Count) {
                    std::vector<ID3D11Texture2D*> images[xr::StereoView::Count];
                    pvrViewPort viewports[xr::StereoView::Count]{};
                    pvrFovPort fovs[xr::StereoView::Count]{};
                    bool valid = true;
                    for (uint32_t eye = 0; eye < xr::StereoView::Count && valid; eye++) {
                        const XrCompositionLayerProjectionView& view = projection->views[eye];
                        valid = getSwapchainImages(view.subImage.swapchain, images[eye]);
                        viewports[eye].x = view.subImage.imageRect.offset.x;
                        viewports[eye].y = view.subImage.imageRect.offset.y;
                        viewports[eye].width = view.subImage.imageRect.extent.width;
                        viewports[eye].height = view.subImage.imageRect.extent.height;
                        fovs[eye] = ToPvrFovPort(view.fov);
                    }

                    if (valid) {
                        // Texture arrays (e.g. Unity single-pass instanced): both views use the same swapchain, one
                        // array slice each. LibMagic it applies a single shading-rate pattern to both slices. Give
                        // it the average of both FOVs (in tangent space, where texture coordinates are linear) so
                        // the full-detail area lands halfway between where the gaze projects in each eye. That
                        // leaves each eye off by half the (horizontal) gap between eyes, so also stretch the area
                        // horizontally (see m_isTextureArray).
                        const bool isTextureArray =
                            projection->views[0].subImage.swapchain == projection->views[1].subImage.swapchain &&
                            projection->views[0].subImage.imageArrayIndex !=
                                projection->views[1].subImage.imageArrayIndex;
                        if (isTextureArray) {
                            pvrFovPort combined{};
                            combined.UpTan = (fovs[0].UpTan + fovs[1].UpTan) / 2.f;
                            combined.DownTan = (fovs[0].DownTan + fovs[1].DownTan) / 2.f;
                            combined.LeftTan = (fovs[0].LeftTan + fovs[1].LeftTan) / 2.f;
                            combined.RightTan = (fovs[0].RightTan + fovs[1].RightTan) / 2.f;

                            // Horizontal range is [-LeftTan, RightTan]: shrink it by k around its center.
                            const float k = 1.f / (std::max)(m_textureArrayStretch.load(), 1.f);
                            const float centerX = (combined.RightTan - combined.LeftTan) / 2.f;
                            combined.LeftTan = k * (centerX + combined.LeftTan) - centerX;
                            combined.RightTan = centerX + k * (combined.RightTan - centerX);
                            m_textureArrayCenterX = centerX;
                            m_textureArrayGazeScaleX = k;

                            fovs[0] = fovs[1] = combined;
                        }
                        if (m_isTextureArray.exchange(isTextureArray) != isTextureArray) {
                            Log(fmt::format("Combining foveation for texture arrays: {}\n",
                                            isTextureArray ? "on" : "off"));
                        }

                        // Left eye images in the first half, right eye images in the second half. LibMagic expects
                        // at least k_MinImagesPerEye images per eye, so we pad with nullptr if the swapchain
                        const uint32_t imagesPerEye = (std::max)({k_MinImagesPerEye,
                                                                  static_cast<uint32_t>(images[0].size()),
                                                                  static_cast<uint32_t>(images[1].size())});
                        std::vector<ID3D11Texture2D*> textures(2 * imagesPerEye, nullptr);
                        for (uint32_t eye = 0; eye < xr::StereoView::Count; eye++) {
                            std::copy(images[eye].begin(), images[eye].end(), textures.begin() + eye * imagesPerEye);
                        }

                        m_submitEyeTextures(textures.data(),
                                            static_cast<int>(textures.size()),
                                            viewports,
                                            fovs,
                                            false /* originBottomLeft */);
                    }
                }
            }
            lock.unlock();

            const XrResult result = m_xrEndFrame(session, frameEndInfo);
            if (XR_SUCCEEDED(result)) {
                lock.lock();
                countFrameSubmitted();
            }

            TraceLoggingWriteStop(local, "xrEndFrame", TLArg(xr::ToCString(result), "Result"));
            return result;
        }

        XrResult xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
            const XrResult result = m_xrPollEvent(instance, eventData);

            std::unique_lock lock(m_mutex);
            m_needPollEvent = false;

            return result;
        }

        XrResult xrSuggestInteractionProfileBindings(XrInstance instance,
                                                     const XrInteractionProfileSuggestedBinding* suggestedBindings) {
            const XrResult result = m_xrSuggestInteractionProfileBindings(instance, suggestedBindings);
            if (XR_SUCCEEDED(result) && suggestedBindings) {
                std::unique_lock lock(m_mutex);
                // Each call replaces the previous suggestions for that interaction profile. Remember the application's
                // latest bindings for the eye gaze interaction profile, so we can add ours to them instead of replacing
                // them (see attachSessionActionSets()).
                if (instance == m_instance && suggestedBindings->interactionProfile == getEyeGazeProfilePath()) {
                    m_appEyeGazeBindings.assign(suggestedBindings->suggestedBindings,
                                                suggestedBindings->suggestedBindings +
                                                    suggestedBindings->countSuggestedBindings);
                }
            }
            return result;
        }

        XrResult xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo) {
            if (!attachInfo) {
                return m_xrAttachSessionActionSets(session, attachInfo);
            }

            std::unique_lock lock(m_mutex);
            // Inject our own actionset (and suggested bindings) if needed.
            return attachSessionActionSets(session, attachInfo);
        }

        XrResult xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
            if (!syncInfo) {
                return m_xrSyncActions(session, syncInfo);
            }

            std::unique_lock lock(m_mutex);

            std::vector<XrActiveActionSet> activeActionSets;
            XrActionsSyncInfo chainSyncInfo = *syncInfo;
            // Inject our own actionset if needed.
            if (isSessionSupported(session) && m_isEyeGazeSupported && m_submitEyeTextures) {
                activeActionSets.assign(chainSyncInfo.activeActionSets,
                                        chainSyncInfo.activeActionSets + chainSyncInfo.countActiveActionSets);
                activeActionSets.push_back({m_eyeTrackerActionSet, XR_NULL_PATH});

                chainSyncInfo.activeActionSets = activeActionSets.data();
                chainSyncInfo.countActiveActionSets = static_cast<uint32_t>(activeActionSets.size());
            }

            const XrResult result = m_xrSyncActions(session, &chainSyncInfo);

            m_needSyncActions = false;

            return result;
        }

        XrResult xrCreateApiLayerInstance(const XrInstanceCreateInfo* instanceCreateInfo,
                                          const XrApiLayerCreateInfo* apiLayerInfo,
                                          XrInstance* instance) {
            if (!apiLayerInfo || apiLayerInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO ||
                apiLayerInfo->structVersion != XR_API_LAYER_CREATE_INFO_STRUCT_VERSION ||
                apiLayerInfo->structSize != sizeof(XrApiLayerCreateInfo) || !apiLayerInfo->nextInfo ||
                apiLayerInfo->nextInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO ||
                apiLayerInfo->nextInfo->structVersion != XR_API_LAYER_NEXT_INFO_STRUCT_VERSION ||
                apiLayerInfo->nextInfo->structSize != sizeof(XrApiLayerNextInfo) ||
                !apiLayerInfo->nextInfo->nextGetInstanceProcAddr ||
                !apiLayerInfo->nextInfo->nextCreateApiLayerInstance) {
                return XR_ERROR_INITIALIZATION_FAILED;
            }

            m_xrGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;

            // Request the eye gaze interaction extension (if available), in order to do eye tracking.
            std::vector<const char*> enabledExtensionNames(instanceCreateInfo->enabledExtensionNames,
                                                           instanceCreateInfo->enabledExtensionNames +
                                                               instanceCreateInfo->enabledExtensionCount);
            const auto isEyeGazeExtension = [](const char* name) {
                return std::string_view(name) == XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME;
            };
            bool isEyeGazeExtensionEnabled =
                std::any_of(enabledExtensionNames.cbegin(), enabledExtensionNames.cend(), isEyeGazeExtension);
            if (!isEyeGazeExtensionEnabled && isEyeGazeExtensionAvailable(instanceCreateInfo, apiLayerInfo)) {
                Log(fmt::format("Requesting extension: {}\n", XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME));
                enabledExtensionNames.push_back(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME);
                isEyeGazeExtensionEnabled = true;
            }
            XrInstanceCreateInfo chainInstanceCreateInfo = *instanceCreateInfo;
            chainInstanceCreateInfo.enabledExtensionNames = enabledExtensionNames.data();
            chainInstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());

            // Call down the chain with the next layer's info.
            XrApiLayerCreateInfo chainApiLayerInfo = *apiLayerInfo;
            chainApiLayerInfo.nextInfo = apiLayerInfo->nextInfo->next;
            const XrResult result = apiLayerInfo->nextInfo->nextCreateApiLayerInstance(
                &chainInstanceCreateInfo, &chainApiLayerInfo, instance);
            if (XR_FAILED(result)) {
                return result;
            }

            // Resolve the functions that we call, intercepted or not.
            std::unique_lock lock(m_mutex);
            m_instance = *instance;
            m_isEyeGazeExtensionEnabled = isEyeGazeExtensionEnabled;
#define RESOLVE_NEXT_FUNCTION(name)                                                                                    \
    if (XR_FAILED(m_xrGetInstanceProcAddr(*instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_##name)))) {      \
        m_##name = nullptr;                                                                                            \
        Log("Failed to resolve " #name "\n");                                                                          \
    }
            OPENXR_LAYER_NEXT_FUNCTIONS(RESOLVE_NEXT_FUNCTION)
            OPENXR_LAYER_HOOKED_FUNCTIONS(RESOLVE_NEXT_FUNCTION)
#undef RESOLVE_NEXT_FUNCTION

            publishApplication(instanceCreateInfo->applicationInfo.applicationName);
            m_isOpenXrApplication = true;

            return result;
        }

        XrResult xrDestroyInstance(XrInstance instance) {
            {
                std::unique_lock lock(m_mutex);
                unpublishApplication();
                m_eyeGazeProfilePath = XR_NULL_PATH;
                m_appEyeGazeBindings.clear();
            }
            return m_xrDestroyInstance(instance);
        }

        XrResult xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
            const XrResult result = m_xrGetInstanceProcAddr ? m_xrGetInstanceProcAddr(instance, name, function)
                                                            : XR_ERROR_FUNCTION_UNSUPPORTED;
            if (XR_SUCCEEDED(result) && name && function && *function) {
                const std::string_view apiName(name);

                // Intercept the calls shimmed by our layer.
                if (apiName == "xrDestroyInstance") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrDestroyInstance);
                } else if (apiName == "xrPollEvent") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrPollEvent);
                } else if (apiName == "xrCreateSession") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrCreateSession);
                } else if (apiName == "xrDestroySession") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrDestroySession);
                } else if (apiName == "xrBeginSession") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrBeginSession);
                } else if (apiName == "xrSuggestInteractionProfileBindings") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrSuggestInteractionProfileBindings);
                } else if (apiName == "xrAttachSessionActionSets") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrAttachSessionActionSets);
                } else if (apiName == "xrSyncActions") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrSyncActions);
                } else if (apiName == "xrWaitFrame") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrWaitFrame);
                } else if (apiName == "xrBeginFrame") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrBeginFrame);
                } else if (apiName == "xrEndFrame") {
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrEndFrame);
                }
            }

            return result;
        }

      private:
        OpenXrLayer() = default;

        // Loads LibMagic in OpenXR mode once DFR-UI is running. Called with m_mutex held.
        void tryLoadLibMagic() {
            if (m_libMagicLoadAttempted) {
                return;
            }

            const ULONGLONG now = GetTickCount64();
            if (now - m_lastDfrUiCheck < k_DfrUiCheckIntervalMs) {
                return;
            }
            m_lastDfrUiCheck = now;

            if (!IsDfrUiRunning()) {
                return;
            }

            m_libMagicLoadAttempted = true;
            const auto libMagicPath = GetModuleDirectory() / k_LibMagicName;
            Log(fmt::format("DFR-UI detected, loading {}\n", libMagicPath.string()));
            m_libMagic = LoadLibraryW(libMagicPath.c_str());
            if (!m_libMagic) {
                Log(fmt::format("Failed to load {}: {}\n", libMagicPath.string(), GetLastError()));
                return;
            }

            m_submitEyeTextures =
                reinterpret_cast<PFN_submitEyeTextures>(GetProcAddress(m_libMagic, "submitEyeTextures"));
            if (!m_submitEyeTextures) {
                Log("Failed to find submitEyeTextures in LibMagic\n");
            }
            TraceLoggingWrite(g_traceProvider, "LibMagic_Loaded", TLArg(!!m_submitEyeTextures, "HasSubmitEyeTextures"));
        }

        // While the OpenXR standard states that xrEnumerateInstanceExtensionProperties() can be queried without an
        // instance, this does not stand for API layers, since API layers implementation might rely on the next
        // xrGetInstanceProcAddr() pointer, which is not (yet) populated if no instance is created.
        // We create a dummy instance in order to do these checks.
        static bool isEyeGazeExtensionAvailable(const XrInstanceCreateInfo* instanceCreateInfo,
                                                const XrApiLayerCreateInfo* apiLayerInfo) {
            XrInstance dummyInstance = XR_NULL_HANDLE;

            // Call the chain to create a dummy instance. Request no extensions in order to speed things up.
            XrInstanceCreateInfo dummyCreateInfo = *instanceCreateInfo;
            dummyCreateInfo.enabledExtensionCount = 0;

            XrApiLayerCreateInfo chainApiLayerInfo = *apiLayerInfo;
            chainApiLayerInfo.nextInfo = apiLayerInfo->nextInfo->next;
            if (XR_FAILED(apiLayerInfo->nextInfo->nextCreateApiLayerInstance(
                    &dummyCreateInfo, &chainApiLayerInfo, &dummyInstance))) {
                return false;
            }

            const auto getProcAddr = [&](const char* name, auto& function) {
                if (XR_FAILED(apiLayerInfo->nextInfo->nextGetInstanceProcAddr(
                        dummyInstance, name, reinterpret_cast<PFN_xrVoidFunction*>(&function)))) {
                    function = nullptr;
                }
            };
            PFN_xrDestroyInstance xrDestroyInstance = nullptr;
            getProcAddr("xrDestroyInstance", xrDestroyInstance);
            PFN_xrGetSystem xrGetSystem = nullptr;
            getProcAddr("xrGetSystem", xrGetSystem);
            PFN_xrGetSystemProperties xrGetSystemProperties = nullptr;
            getProcAddr("xrGetSystemProperties", xrGetSystemProperties);
            PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties = nullptr;
            getProcAddr("xrEnumerateInstanceExtensionProperties", xrEnumerateInstanceExtensionProperties);

            // Check the available extensions.
            bool isAvailable = false;
            uint32_t extensionsCount = 0;
            if (xrEnumerateInstanceExtensionProperties &&
                XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionsCount, nullptr))) {
                std::vector<XrExtensionProperties> extensions(extensionsCount, {XR_TYPE_EXTENSION_PROPERTIES});
                if (XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(
                        nullptr, extensionsCount, &extensionsCount, extensions.data()))) {
                    isAvailable = std::any_of(extensions.cbegin(), extensions.cend(), [](const auto& properties) {
                        return std::string_view(properties.extensionName) == XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME;
                    });
                }
            }
            if (!isAvailable) {
                Log(fmt::format("Cannot satisfy implicit extension request: {}\n",
                                XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME));
            }

            // Workaround: the Vive runtime does not seem to like our flow of destroying the instance
            // mid-initialization. We skip destruction and we will just create a second instance.
            if (xrGetSystem && xrGetSystemProperties) {
                XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
                getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
                XrSystemId systemId;
                if (XR_SUCCEEDED(xrGetSystem(dummyInstance, &getInfo, &systemId))) {
                    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
                    if (XR_SUCCEEDED(xrGetSystemProperties(dummyInstance, systemId, &systemProperties)) &&
                        std::string_view(systemProperties.systemName).find("Vive Reality system") !=
                            std::string_view::npos) {
                        xrDestroyInstance = nullptr;
                    }
                }
            }
            if (xrDestroyInstance) {
                xrDestroyInstance(dummyInstance);
            }

            return isAvailable;
        }

        bool isEyeGazeInteractionSupported() {
            XrSystemGetInfo getInfo{XR_TYPE_SYSTEM_GET_INFO};
            getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
            XrSystemId systemId = XR_NULL_SYSTEM_ID;
            if (XR_FAILED(m_xrGetSystem(m_instance, &getInfo, &systemId))) {
                return false;
            }

            // Check if the system supports eye tracking.
            XrSystemEyeGazeInteractionPropertiesEXT eyeGazeInteractionProperties{
                XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT};
            XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES, &eyeGazeInteractionProperties};
            if (XR_FAILED(m_xrGetSystemProperties(m_instance, systemId, &systemProperties))) {
                return false;
            }
            TraceLoggingWrite(
                g_traceProvider,
                "EyeGazeInteraction",
                TLArg(eyeGazeInteractionProperties.supportsEyeGazeInteraction, "SupportsEyeGazeInteraction"));

            return eyeGazeInteractionProperties.supportsEyeGazeInteraction;
        }

        bool initializeEyeGazeInteraction(XrSession session) {
            if (m_eyeTrackerActionSet == XR_NULL_HANDLE) {
                XrActionSetCreateInfo actionSetCreateInfo{XR_TYPE_ACTION_SET_CREATE_INFO, nullptr};
                strcpy_s(actionSetCreateInfo.actionSetName, "pimax_magic4all_eye_tracker");
                strcpy_s(actionSetCreateInfo.localizedActionSetName, "Eye Tracker");
                actionSetCreateInfo.priority = 0;
                if (XR_FAILED(m_xrCreateActionSet(m_instance, &actionSetCreateInfo, &m_eyeTrackerActionSet))) {
                    Log("Failed to create the eye tracker action set\n");
                    m_eyeTrackerActionSet = XR_NULL_HANDLE;
                    return false;
                }

                XrActionCreateInfo actionCreateInfo{XR_TYPE_ACTION_CREATE_INFO, nullptr};
                strcpy_s(actionCreateInfo.actionName, "pimax_magic4all_eye_tracker");
                strcpy_s(actionCreateInfo.localizedActionName, "Eye Tracker");
                actionCreateInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
                actionCreateInfo.countSubactionPaths = 0;
                if (XR_FAILED(m_xrCreateAction(m_eyeTrackerActionSet, &actionCreateInfo, &m_eyeGazeAction))) {
                    Log("Failed to create the eye tracker action\n");
                    m_eyeGazeAction = XR_NULL_HANDLE;
                    return false;
                }
            }

            XrActionSpaceCreateInfo actionSpaceCreateInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO, nullptr};
            actionSpaceCreateInfo.action = m_eyeGazeAction;
            actionSpaceCreateInfo.subactionPath = XR_NULL_PATH;
            actionSpaceCreateInfo.poseInActionSpace.orientation.w = 1.f;
            XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
            spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.f;
            if (XR_FAILED(m_xrCreateActionSpace(session, &actionSpaceCreateInfo, &m_eyeSpace)) ||
                XR_FAILED(m_xrCreateReferenceSpace(session, &spaceCreateInfo, &m_viewSpace))) {
                Log("Failed to create the eye tracker spaces\n");
                return false;
            }

            TraceLoggingWrite(g_traceProvider, "EyeGazeInteraction_Initialized");
            return true;
        }

        XrResult attachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo* attachInfo) {
            XrSessionActionSetsAttachInfo chainAttachInfo = *attachInfo;
            std::vector<XrActionSet> actionSets;
            if (session == m_session && m_isEyeGazeSupported) {
                // Suggest the bindings for the eye tracker. A suggestion replaces any previous one for the same
                // interaction profile, so include the application's own bindings for it (if any). Applications cannot
                // suggest bindings once action sets are attached, so this is the final set.
                XrActionSuggestedBinding binding;
                binding.action = m_eyeGazeAction;

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
                                                                       nullptr};
                suggestedBindings.interactionProfile = getEyeGazeProfilePath();
                if (suggestedBindings.interactionProfile != XR_NULL_PATH &&
                    XR_SUCCEEDED(
                        m_xrStringToPath(m_instance, "/user/eyes_ext/input/gaze_ext/pose", &binding.binding))) {
                    std::vector<XrActionSuggestedBinding> bindings = m_appEyeGazeBindings;
                    bindings.push_back(binding);
                    suggestedBindings.suggestedBindings = bindings.data();
                    suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                    const XrResult suggestResult =
                        m_xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings);
                    if (XR_FAILED(suggestResult)) {
                        Log(fmt::format("Failed to suggest eye tracker bindings: {}\n",
                                        xr::ToString(suggestResult).c_str()));
                    }
                    TraceLoggingWrite(g_traceProvider,
                                      "EyeGazeInteraction_SuggestBindings",
                                      TLArg(m_appEyeGazeBindings.size(), "ApplicationBindings"),
                                      TLArg(xr::ToString(suggestResult).c_str(), "Result"));
                }

                // Inject our actionset.
                actionSets.assign(chainAttachInfo.actionSets,
                                  chainAttachInfo.actionSets + chainAttachInfo.countActionSets);
                actionSets.push_back(m_eyeTrackerActionSet);

                chainAttachInfo.actionSets = actionSets.data();
                chainAttachInfo.countActionSets = static_cast<uint32_t>(actionSets.size());
            }

            const XrResult result = m_xrAttachSessionActionSets(session, &chainAttachInfo);

            m_needAttachActionSets = false;

            return result;
        }

        XrPath getEyeGazeProfilePath() {
            if (m_eyeGazeProfilePath == XR_NULL_PATH && m_instance != XR_NULL_HANDLE && m_xrStringToPath &&
                XR_FAILED(m_xrStringToPath(
                    m_instance, "/interaction_profiles/ext/eye_gaze_interaction", &m_eyeGazeProfilePath))) {
                m_eyeGazeProfilePath = XR_NULL_PATH;
            }
            return m_eyeGazeProfilePath;
        }

        std::optional<XrPosef> getEyeGazeInteraction(XrTime time) {
            XrActionStatePose actionStatePose{XR_TYPE_ACTION_STATE_POSE, nullptr};
            XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr};
            getInfo.action = m_eyeGazeAction;
            const XrResult result = m_xrGetActionStatePose(m_session, &getInfo, &actionStatePose);
            TraceLoggingWrite(g_traceProvider,
                              "EyeGazeInteraction",
                              TLArg(xr::ToCString(result), "Result"),
                              TLArg(!!actionStatePose.isActive, "Active"));

            if (XR_FAILED(result) || !actionStatePose.isActive) {
                return {};
            }

            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION, nullptr};
            if (XR_FAILED(m_xrLocateSpace(m_eyeSpace, m_viewSpace, time, &location))) {
                return {};
            }
            TraceLoggingWrite(g_traceProvider, "EyeGazeInteraction", TLArg(location.locationFlags, "LocationFlags"));

            if (!xr::math::Pose::IsPoseValid(location.locationFlags)) {
                return {};
            }

            return location.pose;
        }

        void publishApplication(const char* applicationName) {
            // If another OpenXR application already published, take over (the most recent one wins).
            m_applicationInfoMapping.reset(CreateFileMappingW(INVALID_HANDLE_VALUE,
                                                              nullptr,
                                                              PAGE_READWRITE,
                                                              0,
                                                              sizeof(SharedApplicationInfo),
                                                              k_ApplicationInfoMappingName));
            if (!m_applicationInfoMapping) {
                Log(fmt::format("Failed to create application info mapping: {}\n", GetLastError()));
                return;
            }
            m_applicationInfo.reset(static_cast<SharedApplicationInfo*>(
                MapViewOfFile(m_applicationInfoMapping.get(), FILE_MAP_WRITE, 0, 0, sizeof(SharedApplicationInfo))));
            if (!m_applicationInfo) {
                Log(fmt::format("Failed to map application info: {}\n", GetLastError()));
                m_applicationInfoMapping.reset();
                return;
            }

            InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_applicationInfo->processId), 0);
            m_applicationInfo->version = k_ApplicationInfoVersion;
            strncpy_s(m_applicationInfo->applicationName, applicationName, _TRUNCATE);
            InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&m_applicationInfo->cpuFrameTimeSumUs), 0);
            InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&m_applicationInfo->gpuFrameTimeSumUs), 0);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_applicationInfo->cpuFrameTimeSamples), 0);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_applicationInfo->gpuFrameTimeSamples), 0);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_applicationInfo->framesSubmitted), 0);
            InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_applicationInfo->processId),
                                static_cast<LONG>(GetCurrentProcessId()));
        }

        void publishGpuFrameTime(uint64_t gpuFrameTimeUs) {
            if (m_applicationInfo && m_applicationInfo->processId == GetCurrentProcessId()) {
                addSample(m_applicationInfo->gpuFrameTimeSumUs, m_applicationInfo->gpuFrameTimeSamples, gpuFrameTimeUs);
            }
        }

        static void addSample(uint64_t& sum, uint32_t& samples, uint64_t value) {
            InterlockedExchangeAdd64(reinterpret_cast<volatile LONG64*>(&sum), static_cast<LONG64>(value));
            InterlockedIncrement(reinterpret_cast<volatile LONG*>(&samples));
        }

        // Collects the oldest GPU timer and reuses it for the frame starting now.
        void startGpuTimer() {
            if (!m_gpuTimers[0]) {
                return;
            }

            if (m_isGpuTimerRunning) {
                // The previous frame was never ended (discarded frame): restart the same timer.
                m_gpuTimers[m_currentGpuTimer]->stop();
            } else {
                const uint64_t gpuFrameTimeUs = m_gpuTimers[m_currentGpuTimer]->query();
                if (gpuFrameTimeUs) {
                    publishGpuFrameTime(gpuFrameTimeUs);
                    TraceLoggingWrite(g_traceProvider, "GpuFrameTime", TLArg(gpuFrameTimeUs, "AppGpuFrameTimeUs"));
                }
            }

            m_gpuTimers[m_currentGpuTimer]->start();
            m_isGpuTimerRunning = true;
        }

        void publishCpuFrameTime(uint64_t cpuFrameTimeUs) {
            if (m_applicationInfo && m_applicationInfo->processId == GetCurrentProcessId()) {
                addSample(m_applicationInfo->cpuFrameTimeSumUs, m_applicationInfo->cpuFrameTimeSamples, cpuFrameTimeUs);
            }
        }

        void countFrameSubmitted() {
            if (m_applicationInfo && m_applicationInfo->processId == GetCurrentProcessId()) {
                InterlockedIncrement(reinterpret_cast<volatile LONG*>(&m_applicationInfo->framesSubmitted));
            }
        }

        void unpublishApplication() {
            if (m_applicationInfo && m_applicationInfo->processId == GetCurrentProcessId()) {
                InterlockedExchange(reinterpret_cast<volatile LONG*>(&m_applicationInfo->processId), 0);
            }
            m_applicationInfo.reset();
            m_applicationInfoMapping.reset();
        }

        bool getSwapchainImages(XrSwapchain swapchain, std::vector<ID3D11Texture2D*>& images) {
            uint32_t count = 0;
            if (XR_FAILED(m_xrEnumerateSwapchainImages(swapchain, 0, &count, nullptr))) {
                return false;
            }
            std::vector<XrSwapchainImageD3D11KHR> d3d11Images(count, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
            if (XR_FAILED(m_xrEnumerateSwapchainImages(
                    swapchain, count, &count, reinterpret_cast<XrSwapchainImageBaseHeader*>(d3d11Images.data())))) {
                return false;
            }
            images.clear();
            for (uint32_t i = 0; i < count; i++) {
                images.push_back(d3d11Images[i].texture);
            }
            return true;
        }

        bool isSessionSupported(XrSession session) {
            return session == m_session && m_isStereoSession;
        }

        std::mutex m_mutex;

        // Next in the chain (see OPENXR_LAYER_NEXT_FUNCTIONS and OPENXR_LAYER_HOOKED_FUNCTIONS).
        PFN_xrGetInstanceProcAddr m_xrGetInstanceProcAddr = nullptr;
#define DECLARE_NEXT_FUNCTION(name) PFN_##name m_##name = nullptr;
        OPENXR_LAYER_NEXT_FUNCTIONS(DECLARE_NEXT_FUNCTION)
        OPENXR_LAYER_HOOKED_FUNCTIONS(DECLARE_NEXT_FUNCTION)
#undef DECLARE_NEXT_FUNCTION

        XrInstance m_instance = XR_NULL_HANDLE;

        // The XrSession we hooked.
        XrSession m_session = XR_NULL_HANDLE;

        // Immutable session state that we need toinject (or not).
        ID3D11Device* m_d3d11Device = nullptr;
        bool m_isStereoSession = false;

        // Application GPU frame time.
        std::unique_ptr<D3D11GpuTimer> m_gpuTimers[k_NumGpuTimers];
        uint32_t m_currentGpuTimer = 0;
        bool m_isGpuTimerRunning = false;

        // Application CPU frame time: when the current frame was begun.
        std::optional<std::chrono::steady_clock::time_point> m_frameBeginTime;

        // LibMagic.
        bool m_libMagicLoadAttempted = false;
        HMODULE m_libMagic = nullptr;
        PFN_submitEyeTextures m_submitEyeTextures = nullptr;
        ULONGLONG m_lastDfrUiCheck = 0;

        wil::unique_handle m_applicationInfoMapping;
        wil::unique_mapview_ptr<SharedApplicationInfo> m_applicationInfo;

        std::atomic<bool> m_isOpenXrApplication = false;

        // Eye tracking (XR_EXT_eye_gaze_interaction).
        bool m_isEyeGazeExtensionEnabled = false;
        XrActionSet m_eyeTrackerActionSet = XR_NULL_HANDLE;
        XrAction m_eyeGazeAction = XR_NULL_HANDLE;
        XrSpace m_eyeSpace = XR_NULL_HANDLE;
        XrSpace m_viewSpace = XR_NULL_HANDLE;
        XrPath m_eyeGazeProfilePath = XR_NULL_PATH;
        std::vector<XrActionSuggestedBinding> m_appEyeGazeBindings;
        std::atomic<bool> m_isEyeGazeSupported = false;
        bool m_needPollEvent = true;
        bool m_needAttachActionSets = true;
        bool m_needSyncActions = true;
        uint64_t m_framesElapsed = 0;

        mutable std::mutex m_eyeGazeMutex;
        std::optional<XrPosef> m_eyeGaze;

        std::atomic<bool> m_isTextureArray = false;
        std::atomic<float> m_textureArrayStretch = 1.5f;
        std::atomic<float> m_textureArrayCenterX = 0.f;
        std::atomic<float> m_textureArrayGazeScaleX = 1.f;
    };

} // namespace

namespace openxr_layer {

    IOpenXrLayer& GetInstance() {
        return OpenXrLayer::Get();
    }

} // namespace openxr_layer

XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo* const loaderInfo,
                                                       const char* const apiLayerName,
                                                       XrNegotiateApiLayerRequest* const apiLayerRequest) {
    if (!loaderInfo || !apiLayerRequest || loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
        apiLayerRequest->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
        apiLayerRequest->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
        apiLayerRequest->structSize != sizeof(XrNegotiateApiLayerRequest) ||
        loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxApiVersion < XR_CURRENT_API_VERSION || loaderInfo->minApiVersion > XR_CURRENT_API_VERSION) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }

    // Setup our layer to intercept OpenXR calls.
    apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
    apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
    apiLayerRequest->getInstanceProcAddr = hook_xrGetInstanceProcAddr;
    apiLayerRequest->createApiLayerInstance = hook_xrCreateApiLayerInstance;

    return XR_SUCCESS;
}

namespace {

    // Trampolines from the OpenXR entry points to the singleton.
    XrResult XRAPI_CALL hook_xrGetInstanceProcAddr(XrInstance instance,
                                                   const char* name,
                                                   PFN_xrVoidFunction* function) {
        return OpenXrLayer::Get().xrGetInstanceProcAddr(instance, name, function);
    }

    XrResult XRAPI_CALL hook_xrCreateApiLayerInstance(const XrInstanceCreateInfo* instanceCreateInfo,
                                                      const XrApiLayerCreateInfo* apiLayerInfo,
                                                      XrInstance* instance) {
        return OpenXrLayer::Get().xrCreateApiLayerInstance(instanceCreateInfo, apiLayerInfo, instance);
    }

    XrResult XRAPI_CALL hook_xrDestroyInstance(XrInstance instance) {
        return OpenXrLayer::Get().xrDestroyInstance(instance);
    }

    XrResult XRAPI_CALL hook_xrPollEvent(XrInstance instance, XrEventDataBuffer* eventData) {
        return OpenXrLayer::Get().xrPollEvent(instance, eventData);
    }

    XrResult XRAPI_CALL hook_xrCreateSession(XrInstance instance,
                                             const XrSessionCreateInfo* createInfo,
                                             XrSession* session) {
        return OpenXrLayer::Get().xrCreateSession(instance, createInfo, session);
    }

    XrResult XRAPI_CALL hook_xrDestroySession(XrSession session) {
        return OpenXrLayer::Get().xrDestroySession(session);
    }

    XrResult XRAPI_CALL hook_xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo) {
        return OpenXrLayer::Get().xrBeginSession(session, beginInfo);
    }

    XrResult XRAPI_CALL
    hook_xrSuggestInteractionProfileBindings(XrInstance instance,
                                             const XrInteractionProfileSuggestedBinding* suggestedBindings) {
        return OpenXrLayer::Get().xrSuggestInteractionProfileBindings(instance, suggestedBindings);
    }

    XrResult XRAPI_CALL hook_xrAttachSessionActionSets(XrSession session,
                                                       const XrSessionActionSetsAttachInfo* attachInfo) {
        return OpenXrLayer::Get().xrAttachSessionActionSets(session, attachInfo);
    }

    XrResult XRAPI_CALL hook_xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
        return OpenXrLayer::Get().xrSyncActions(session, syncInfo);
    }

    XrResult XRAPI_CALL hook_xrWaitFrame(XrSession session,
                                         const XrFrameWaitInfo* frameWaitInfo,
                                         XrFrameState* frameState) {
        return OpenXrLayer::Get().xrWaitFrame(session, frameWaitInfo, frameState);
    }

    XrResult XRAPI_CALL hook_xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
        return OpenXrLayer::Get().xrBeginFrame(session, frameBeginInfo);
    }

    XrResult XRAPI_CALL hook_xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
        return OpenXrLayer::Get().xrEndFrame(session, frameEndInfo);
    }

} // namespace
