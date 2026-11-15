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

#include "log.h"
#include "util.h"
#include "gpu_timer.h"
#include "openxr_layer.h"

// OpenXR functions that we call but do not intercept.
// clang-format off
#define OPENXR_LAYER_NEXT_FUNCTIONS(_)  \
    _(xrEnumerateSwapchainImages)
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
    XrResult XRAPI_CALL hook_xrCreateSession(XrInstance instance,
                                             const XrSessionCreateInfo* createInfo,
                                             XrSession* session);
    XrResult XRAPI_CALL hook_xrDestroySession(XrSession session);
    XrResult XRAPI_CALL hook_xrBeginSession(XrSession session, const XrSessionBeginInfo* beginInfo);
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
                } else {
                    Log("Session is not D3D11, using LibMagic is not possible\n");
                }
                TraceLoggingWriteTagged(local, "xrCreateSession", TLArg(!!entry, "IsD3D11"));
            }

            TraceLoggingWriteStop(local, "xrCreateSession", TLArg((int)result, "Result"));
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
                }
            }

            TraceLoggingWriteStop(local, "xrDestroySession", TLArg((int)result, "Result"));
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
                                        (int)beginInfo->primaryViewConfigurationType));
                    }
                }
                TraceLoggingWriteTagged(
                    local, "xrBeginSession", TLArg((int)beginInfo->primaryViewConfigurationType, "ViewConfiguration"));
            }

            TraceLoggingWriteStop(local, "xrBeginSession", TLArg((int)result, "Result"));
            return result;
        }

        XrResult xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrBeginFrame");

            const XrResult result = m_xrBeginFrame(session, frameBeginInfo);
            if (XR_SUCCEEDED(result)) {
                std::unique_lock lock(m_mutex);

                if (session == m_session) {
                    // The application renders between xrBeginFrame() and xrEndFrame(). We do not measure from
                    // xrWaitFrame(): pipelined applications (e.g. Unreal, XrSceneLib) call it from another thread, ahead
                    // of rendering, which would measure their pipeline latency instead. A frame that was never ended
                    // (XR_FRAME_DISCARDED) is simply restarted here.
                    m_frameBeginTime = std::chrono::steady_clock::now();

                    startGpuTimer();
                }
            }

            TraceLoggingWriteStop(local, "xrBeginFrame", TLArg((int)result, "Result"));
            return result;
        }

        XrResult xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
            TraceLocalActivity(local);
            TraceLoggingWriteStart(local, "xrEndFrame");

            const auto now = std::chrono::steady_clock::now();
            {
                std::unique_lock lock(m_mutex);

                // See xrBeginFrame().
                if (session == m_session && m_frameBeginTime) {
                    const auto cpuFrameTimeUs =
                        std::chrono::duration_cast<std::chrono::microseconds>(now - *m_frameBeginTime).count();
                    publishCpuFrameTime(static_cast<uint64_t>(cpuFrameTimeUs));
                    TraceLoggingWriteTagged(local, "xrEndFrame", TLArg(cpuFrameTimeUs, "AppCpuFrameTimeUs"));
                    m_frameBeginTime.reset();
                }

                if (session == m_session && m_isGpuTimerRunning) {
                    m_gpuTimers[m_currentGpuTimer]->stop();
                    m_isGpuTimerRunning = false;
                    m_currentGpuTimer = (m_currentGpuTimer + 1) % k_NumGpuTimers;
                }

                if (session == m_session && m_isStereoSession) {
                    tryLoadLibMagic();

                    // Hand the eye textures of the first projection layer to LibMagic.
                    const XrCompositionLayerProjection* projection = nullptr;
                    if (m_submitEyeTextures && frameEndInfo && m_xrEnumerateSwapchainImages) {
                        for (uint32_t i = 0; i < frameEndInfo->layerCount; i++) {
                            if (frameEndInfo->layers[i] &&
                                frameEndInfo->layers[i]->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION) {
                                projection =
                                    reinterpret_cast<const XrCompositionLayerProjection*>(frameEndInfo->layers[i]);
                                break;
                            }
                        }
                    }

                    if (projection && projection->viewCount >= 2) {
                        std::vector<ID3D11Texture2D*> images[2];
                        pvrViewPort viewports[2]{};
                        pvrFovPort fovs[2]{};
                        bool valid = true;
                        for (uint32_t eye = 0; eye < 2 && valid; eye++) {
                            const XrCompositionLayerProjectionView& view = projection->views[eye];
                            valid = getSwapchainImages(view.subImage.swapchain, images[eye]);
                            viewports[eye].x = view.subImage.imageRect.offset.x;
                            viewports[eye].y = view.subImage.imageRect.offset.y;
                            viewports[eye].width = view.subImage.imageRect.extent.width;
                            viewports[eye].height = view.subImage.imageRect.extent.height;
                            fovs[eye] = ToPvrFovPort(view.fov);
                        }

                        // Texture arrays (e.g. Unity single-pass instanced): both views use the same swapchain, one
                        // array slice each. LibMagic it applies a single shading-rate pattern to both slices. Give it
                        // the average of both FOVs (in tangent space, where texture coordinates are linear) so the
                        // full-detail area lands halfway between where the gaze projects in each eye. That leaves each
                        // eye off by half the (horizontal) gap between eyes, so also stretch the area horizontally (see
                        // m_isTextureArray).
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

                        if (valid) {
                            // Left eye images in the first half, right eye images in the second half. LibMagic expects
                            // at least k_MinImagesPerEye images per eye, so we pad with nullptr if the swapchain
                            const uint32_t imagesPerEye = (std::max)({k_MinImagesPerEye,
                                                                      static_cast<uint32_t>(images[0].size()),
                                                                      static_cast<uint32_t>(images[1].size())});
                            std::vector<ID3D11Texture2D*> textures(2 * imagesPerEye, nullptr);
                            for (uint32_t eye = 0; eye < 2; eye++) {
                                std::copy(
                                    images[eye].begin(), images[eye].end(), textures.begin() + eye * imagesPerEye);
                            }

                            m_submitEyeTextures(textures.data(),
                                                static_cast<int>(textures.size()),
                                                viewports,
                                                fovs,
                                                false /* originBottomLeft */);
                        }
                    }
                }
            }

            const XrResult result = m_xrEndFrame(session, frameEndInfo);
            if (XR_SUCCEEDED(result)) {
                std::unique_lock lock(m_mutex);
                countFrameSubmitted();
            }

            TraceLoggingWriteStop(local, "xrEndFrame", TLArg((int)result, "Result"));
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

            // Call down the chain with the next layer's info.
            XrApiLayerCreateInfo chainApiLayerInfo = *apiLayerInfo;
            chainApiLayerInfo.nextInfo = apiLayerInfo->nextInfo->next;
            const XrResult result =
                apiLayerInfo->nextInfo->nextCreateApiLayerInstance(instanceCreateInfo, &chainApiLayerInfo, instance);
            if (XR_FAILED(result)) {
                return result;
            }

            // Resolve the functions that we call but do not intercept.
            std::unique_lock lock(m_mutex);
#define RESOLVE_NEXT_FUNCTION(name)                                                                                    \
    if (XR_FAILED(m_xrGetInstanceProcAddr(*instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_##name)))) {      \
        m_##name = nullptr;                                                                                            \
        Log("Failed to resolve " #name "\n");                                                                          \
    }
            OPENXR_LAYER_NEXT_FUNCTIONS(RESOLVE_NEXT_FUNCTION)
#undef RESOLVE_NEXT_FUNCTION

            publishApplication(instanceCreateInfo->applicationInfo.applicationName);
            m_isOpenXrApplication = true;

            return result;
        }

        XrResult xrDestroyInstance(XrInstance instance) {
            {
                std::unique_lock lock(m_mutex);
                unpublishApplication();
            }
            return m_xrDestroyInstance(instance);
        }

        XrResult xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
            const XrResult result = m_xrGetInstanceProcAddr ? m_xrGetInstanceProcAddr(instance, name, function)
                                                            : XR_ERROR_FUNCTION_UNSUPPORTED;
            if (XR_SUCCEEDED(result) && name && function && *function) {
                const std::string_view apiName(name);

                // Intercept the calls shimmed by our layer.
                if (apiName == "xrCreateSession") {
                    m_xrCreateSession = reinterpret_cast<PFN_xrCreateSession>(*function);
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrCreateSession);
                } else if (apiName == "xrDestroySession") {
                    m_xrDestroySession = reinterpret_cast<PFN_xrDestroySession>(*function);
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrDestroySession);
                } else if (apiName == "xrBeginSession") {
                    m_xrBeginSession = reinterpret_cast<PFN_xrBeginSession>(*function);
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrBeginSession);
                } else if (apiName == "xrBeginFrame") {
                    m_xrBeginFrame = reinterpret_cast<PFN_xrBeginFrame>(*function);
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrBeginFrame);
                } else if (apiName == "xrEndFrame") {
                    m_xrEndFrame = reinterpret_cast<PFN_xrEndFrame>(*function);
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrEndFrame);
                } else if (apiName == "xrDestroyInstance") {
                    m_xrDestroyInstance = reinterpret_cast<PFN_xrDestroyInstance>(*function);
                    *function = reinterpret_cast<PFN_xrVoidFunction>(hook_xrDestroyInstance);
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

        std::mutex m_mutex;

        // Next in the chain.
        PFN_xrGetInstanceProcAddr m_xrGetInstanceProcAddr = nullptr;
        PFN_xrCreateSession m_xrCreateSession = nullptr;
        PFN_xrDestroySession m_xrDestroySession = nullptr;
        PFN_xrBeginSession m_xrBeginSession = nullptr;
        PFN_xrBeginFrame m_xrBeginFrame = nullptr;
        PFN_xrEndFrame m_xrEndFrame = nullptr;
        PFN_xrDestroyInstance m_xrDestroyInstance = nullptr;

        // Next in the chain, not intercepted (see OPENXR_LAYER_NEXT_FUNCTIONS).
#define DECLARE_NEXT_FUNCTION(name) PFN_##name m_##name = nullptr;
        OPENXR_LAYER_NEXT_FUNCTIONS(DECLARE_NEXT_FUNCTION)
#undef DECLARE_NEXT_FUNCTION

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

    XrResult XRAPI_CALL hook_xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo) {
        return OpenXrLayer::Get().xrBeginFrame(session, frameBeginInfo);
    }

    XrResult XRAPI_CALL hook_xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo) {
        return OpenXrLayer::Get().xrEndFrame(session, frameEndInfo);
    }

} // namespace
