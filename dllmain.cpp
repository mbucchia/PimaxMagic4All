// MIT License
//
// Copyright(c) 2023 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in all
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

// {cbf3adcd-42b9-4c38-830b-91980af201f6}
TRACELOGGING_DEFINE_PROVIDER(g_traceProvider,
                             "PvrEmu",
                             (0xcbf3adcd, 0x42b9, 0x4c38, 0x83, 0x0b, 0x91, 0x98, 0x0a, 0xf2, 0x01, 0xf6));

TraceLoggingActivity<g_traceProvider> g_traceActivity;

//
// Tracelogging helpers.
//

#define TraceLocalActivity(activity) TraceLoggingActivity<g_traceProvider> activity;

#define TLArg(var, ...) TraceLoggingValue(var, ##__VA_ARGS__)
#define TLPArg(var, ...) TraceLoggingPointer(var, ##__VA_ARGS__)

//
// Log file helpers.
//

namespace utils {
    namespace {

        std::ofstream logStream;

        // Utility logging function.
        void InternalLog(const char* fmt, va_list va) {
            const std::time_t now = std::time(nullptr);

            char buf[1024];
            size_t offset = std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %z: ", std::localtime(&now));
            vsnprintf_s(buf + offset, sizeof(buf) - offset, _TRUNCATE, fmt, va);
            OutputDebugStringA(buf);
            if (logStream.is_open()) {
                logStream << buf;
                logStream.flush();
            }
        }

    } // namespace

    void Log(const char* fmt, ...) {
        va_list va;
        va_start(va, fmt);
        InternalLog(fmt, va);
        va_end(va);
    }

} // namespace utils

//
// Detours helpers.
//

namespace utils {

    template <typename TMethod>
    void DetourDllAttach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (original) {
            // Already hooked.
            return;
        }

        HMODULE handle;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN, dll, &handle);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        original = (TMethod)GetProcAddress(handle, target);
        DetourAttach((PVOID*)&original, hooked);

        DetourTransactionCommit();
    }

    template <typename TMethod>
    void DetourDllDetach(const char* dll, const char* target, TMethod hooked, TMethod& original) {
        if (!original) {
            // Not hooked.
            return;
        }

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourDetach((PVOID*)&original, hooked);

        DetourTransactionCommit();

        original = nullptr;
    }

} // namespace utils

//
// And now, the real stuff.
//

namespace {

    using namespace utils;

    // Hook to fake process ID.
    decltype(GetCurrentProcessId)* g_original_GetCurrentProcessId = nullptr;
    DWORD WINAPI hooked_GetCurrentProcessId() {
        // We try to only intercept calls from the Varjo client.
        HMODULE callerModule;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)_ReturnAddress(),
                               &callerModule)) {
            HMODULE libVarjo = GetModuleHandleA("VarjoLib.dll");
            HMODULE libVarjoRuntime = GetModuleHandleA("VarjoRuntime.dll");
            if (callerModule != libVarjo && callerModule != libVarjoRuntime) {
                return g_original_GetCurrentProcessId();
            }
        }

        // Always return a process ID different from ours.
        return g_original_GetCurrentProcessId() + 42;
    }

    // Varjo session handle.
    varjo_Session* varjoSession = nullptr;

    // OpenXR handles.
    XrInstance openXrInstance = XR_NULL_HANDLE;
    XrSystemId openXrSystemId = XR_NULL_SYSTEM_ID;
    XrSession openXrSession = XR_NULL_HANDLE;
    XrEyeTrackerFB openXrEyeTrackerFB = XR_NULL_HANDLE;
    XrSpace openXrViewSpace = XR_NULL_HANDLE;

    // OpenXR cache.
    std::thread openXrThread;
    std::mutex openXrCacheMutex;
    std::optional<XrVector3f> openXrCacheGazeUnitVector;

    wil::unique_registry_watcher registryWatcher;
    std::atomic<uint32_t> mode = 0;
    std::atomic<bool> ignoreEyeTracking = 0;

    std::chrono::time_point<std::chrono::steady_clock> lastGoodEyeTrackingDataTime{};
    std::optional<pvrEyeTrackingInfo> lastGoodEyeTrackingInfo;

    void updateMode() {
        DWORD data{};
        DWORD dataSize = sizeof(data);
        LONG retCode;

        retCode = ::RegGetValue(HKEY_CURRENT_USER,
                                L"SOFTWARE\\FR-Utility",
                                L"mode",
                                RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_DWORD,
                                nullptr,
                                &data,
                                &dataSize);
        if (retCode == ERROR_SUCCESS) {
            mode.store(data);
        } else {
            mode.store(0);
        }

        retCode = ::RegGetValue(HKEY_CURRENT_USER,
                                L"SOFTWARE\\FR-Utility",
                                L"ignore_eye_tracking",
                                RRF_SUBKEY_WOW6464KEY | RRF_RT_REG_DWORD,
                                nullptr,
                                &data,
                                &dataSize);
        if (retCode == ERROR_SUCCESS) {
            ignoreEyeTracking.store(data);
        } else {
            ignoreEyeTracking.store(false);
        }
    }

    // Query eye tracker state in the background.
    void openXrEyeTrackerFBThread() {
        XrResult result = XR_ERROR_RUNTIME_FAILURE;

        PFN_xrGetEyeGazesFB pfnXrGetEyeGazesFB = nullptr;
        result = xrGetInstanceProcAddr(openXrInstance, "xrGetEyeGazesFB", (PFN_xrVoidFunction*)&pfnXrGetEyeGazesFB);
        if (XR_FAILED(result)) {
            Log("Failed to retrieve xrGetEyeGazesFB(): %d\n", result);
            return;
        }

        Log("openXrEyeTrackerFBThread started\n");

        while (true) {
            XrFrameState frameState{XR_TYPE_FRAME_STATE};
            result = xrWaitFrame(openXrSession, nullptr, &frameState);
            if (XR_FAILED(result)) {
                Log("xrWaitFrame(): %d\n", result);
                break;
            }

            result = xrBeginFrame(openXrSession, nullptr);
            if (XR_FAILED(result)) {
                Log("xrBeginFrame(): %d\n", result);
                break;
            }

            XrEyeGazesInfoFB getEyeGazesInfo{XR_TYPE_EYE_GAZES_INFO_FB};
            getEyeGazesInfo.baseSpace = openXrViewSpace;
            getEyeGazesInfo.time = frameState.predictedDisplayTime;

            XrEyeGazesFB eyeGazes{XR_TYPE_EYE_GAZES_FB};
            result = pfnXrGetEyeGazesFB(openXrEyeTrackerFB, &getEyeGazesInfo, &eyeGazes);

            {
                static int i = 0;
                i++;
                if (!(i % 200)) {
                    Log("xrGetEyeGazesFB(): %d / %d%d (%.1f %.1f)\n",
                        eyeGazes.gaze[0].isValid,
                        eyeGazes.gaze[1].isValid,
                        eyeGazes.gaze[0].gazeConfidence,
                        eyeGazes.gaze[1].gazeConfidence);
                }
            }

            {
                std::unique_lock lock(openXrCacheMutex);

                if (XR_SUCCEEDED(result) && eyeGazes.gaze[0].isValid && eyeGazes.gaze[1].isValid &&
                    eyeGazes.gaze[0].gazeConfidence > 0.5f && eyeGazes.gaze[1].gazeConfidence > 0.5f) {
                    // Average the poses from both eyes.
                    const auto gaze = xr::math::LoadXrPose(
                        xr::math::Pose::Slerp(eyeGazes.gaze[0].gazePose, eyeGazes.gaze[0].gazePose, 0.5f));
                    const auto gazeProjectedPoint =
                        DirectX::XMVector3Transform(DirectX::XMVectorSet(0.f, 0.f, 1.f, 1.f), gaze);

                    openXrCacheGazeUnitVector = xr::math::Normalize({gazeProjectedPoint.m128_f32[0],
                                                                     gazeProjectedPoint.m128_f32[1],
                                                                     gazeProjectedPoint.m128_f32[2]});
                } else {
                    openXrCacheGazeUnitVector.reset();
                }
            }

            // Don't even submit anything.
        }
    }

    pvrResult emulate_initialise() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_initialize");

        if (!varjoSession && openXrInstance == XR_NULL_HANDLE) {
            if (varjo_IsAvailable()) {
                // Initialize the Varjo SDK.

                const char* version = varjo_GetVersionString();
                TraceLoggingWriteTagged(local, "Varjo_SDK", TLArg(version));
                Log("Varjo SDK: %s\n", version);

                // We will fake process ID to make sure the Varjo SDK doesn't exit SteamVR.
                DetourDllAttach(
                    "kernel32.dll", "GetCurrentProcessId", hooked_GetCurrentProcessId, g_original_GetCurrentProcessId);

                varjoSession = varjo_SessionInit();

                DetourDllDetach(
                    "kernel32.dll", "GetCurrentProcessId", hooked_GetCurrentProcessId, g_original_GetCurrentProcessId);

                TraceLoggingWriteTagged(local, "Varjo_Session", TLPArg(varjoSession));
            } else {
                do {
                    // Or try OpenXR (for a Quest Pro).

                    XrResult result = XR_ERROR_RUNTIME_FAILURE;

                    XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
                    instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
                    sprintf_s(instanceInfo.applicationInfo.applicationName,
                              sizeof(instanceInfo.applicationInfo.applicationName),
                              "PimaxMagic4All");
                    std::vector<const char*> extensions;
                    extensions.push_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
                    extensions.push_back(XR_FB_EYE_TRACKING_SOCIAL_EXTENSION_NAME);
                    instanceInfo.enabledExtensionCount = (uint32_t)extensions.size();
                    instanceInfo.enabledExtensionNames = extensions.data();

                    // We will fail here if the runtime does not support Quest eye tracking.
                    result = xrCreateInstance(&instanceInfo, &openXrInstance);
                    if (XR_FAILED(result)) {
                        Log("xrCreateInstance(): %d\n", result);
                        break;
                    }

                    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
                    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
                    result = xrGetSystem(openXrInstance, &systemInfo, &openXrSystemId);
                    if (XR_FAILED(result)) {
                        Log("xrGetSystem(): %d\n", result);
                        break;
                    }

                    // We must create a graphics session, which is a bit of plumbing...
                    PFN_xrGetD3D11GraphicsRequirementsKHR pfnXrGetD3D11GraphicsRequirementsKHR = nullptr;
                    result = xrGetInstanceProcAddr(openXrInstance,
                                                   "xrGetD3D11GraphicsRequirementsKHR",
                                                   (PFN_xrVoidFunction*)&pfnXrGetD3D11GraphicsRequirementsKHR);
                    if (XR_FAILED(result)) {
                        Log("Failed to retrieve xrGetD3D11GraphicsRequirementsKHR(): %d\n", result);
                        break;
                    }

                    XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
                    result =
                        pfnXrGetD3D11GraphicsRequirementsKHR(openXrInstance, openXrSystemId, &graphicsRequirements);
                    if (XR_FAILED(result)) {
                        Log("xrGetD3D11GraphicsRequirementsKHR(): %d\n", result);
                        break;
                    }

                    Microsoft::WRL::ComPtr<ID3D11Device> device;
                    {
                        HRESULT hr;
                        Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
                        hr = CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf()));
                        if (FAILED(hr)) {
                            Log("CreateDXGIFactory1(): %d\n", result);
                            break;
                        }

                        Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;
                        for (UINT adapterIndex = 0;; adapterIndex++) {
                            hr = dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf());
                            if (FAILED(hr)) {
                                Log("EnumAdapters1(): %d\n", result);
                                break;
                            }

                            DXGI_ADAPTER_DESC1 desc;
                            dxgiAdapter->GetDesc1(&desc);
                            if (!memcmp(&desc.AdapterLuid, &graphicsRequirements.adapterLuid, sizeof(LUID))) {
                                break;
                            }
                        }

                        hr = D3D11CreateDevice(dxgiAdapter.Get(),
                                               D3D_DRIVER_TYPE_UNKNOWN,
                                               0,
                                               0,
                                               &graphicsRequirements.minFeatureLevel,
                                               1,
                                               D3D11_SDK_VERSION,
                                               device.ReleaseAndGetAddressOf(),
                                               nullptr,
                                               nullptr);
                        if (FAILED(hr)) {
                            Log("D3D11CreateDevice(): %d\n", result);
                            break;
                        }
                    }

                    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
                    sessionInfo.systemId = openXrSystemId;
                    XrGraphicsBindingD3D11KHR d3d11Bindings{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
                    d3d11Bindings.device = device.Get();
                    sessionInfo.next = &d3d11Bindings;
                    result = xrCreateSession(openXrInstance, &sessionInfo, &openXrSession);
                    if (XR_FAILED(result)) {
                        Log("xrCreateSession(): %d\n", result);
                        break;
                    }

#if 1
                    // Wait for the session to be ready. This seems to be needed only by the Meta emulator.
                    {
                        while (true) {
                            XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
                            const XrResult result = xrPollEvent(openXrInstance, &event);
                            if (result == XR_SUCCESS) {
                                if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                                    const XrEventDataSessionStateChanged& sessionEvent =
                                        *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);

                                    if (sessionEvent.state == XR_SESSION_STATE_READY) {
                                        break;
                                    }
                                }
                            }

                            // TODO: Need some sort of timeout.
                            if (result == XR_EVENT_UNAVAILABLE) {
                                using namespace std::chrono_literals;
                                std::this_thread::sleep_for(100ms);
                            }
                        }
                    }
#endif

                    XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    result = xrBeginSession(openXrSession, &beginInfo);
                    if (XR_FAILED(result)) {
                        Log("xrBeginSession(): %d\n", result);
                        break;
                    }

                    // And finally the eye tracker resources...
                    PFN_xrCreateEyeTrackerFB pfnXrCreateEyeTrackerFB = nullptr;
                    result = xrGetInstanceProcAddr(
                        openXrInstance, "xrCreateEyeTrackerFB", (PFN_xrVoidFunction*)&pfnXrCreateEyeTrackerFB);
                    if (XR_FAILED(result)) {
                        Log("Failed to retrieve xrCreateEyeTrackerFB(): %d\n", result);
                        break;
                    }

                    XrEyeTrackerCreateInfoFB eyeTrackerInfo{XR_TYPE_EYE_TRACKER_CREATE_INFO_FB};
                    result = pfnXrCreateEyeTrackerFB(openXrSession, &eyeTrackerInfo, &openXrEyeTrackerFB);
                    if (XR_FAILED(result)) {
                        Log("xrCreateEyeTrackerFB(): %d\n", result);
                        if (result == XR_ERROR_RUNTIME_FAILURE) {
                            Log("Did you correctly enable Oculus Developer mode!?\n");
                        }
                        break;
                    }

                    XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
                    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                    spaceCreateInfo.poseInReferenceSpace = xr::math::Pose::Identity();
                    result = xrCreateReferenceSpace(openXrSession, &spaceCreateInfo, &openXrViewSpace);
                    if (XR_FAILED(result)) {
                        Log("xrCreateReferenceSpace(): %d\n", result);
                        break;
                    }
                } while (false);

                if (openXrEyeTrackerFB == XR_NULL_HANDLE) {
                    xrDestroyInstance(openXrInstance);
                    openXrInstance = XR_NULL_HANDLE;
                    openXrSession = XR_NULL_HANDLE;
                    openXrEyeTrackerFB = XR_NULL_HANDLE;
                    openXrViewSpace = XR_NULL_HANDLE;
                }
            }
        }

        // Watch for changes in the registry.
        try {
            wil::unique_hkey keyToWatch;
            if (RegOpenKeyExW(
                    HKEY_CURRENT_USER, L"SOFTWARE\\FR-Utility", 0, KEY_WOW64_64KEY | KEY_READ, keyToWatch.put()) ==
                ERROR_SUCCESS) {
                registryWatcher = wil::make_registry_watcher(
                    std::move(keyToWatch), true, [&](wil::RegistryChangeKind changeType) { updateMode(); });
            }
        } catch (std::exception&) {
            // Ignore errors that can happen with UWP applications not able to write to the registry.
        }

        // Initial reading of the settings.
        updateMode();

        TraceLoggingWriteStop(local, "PVR_initialize");

        // Succeed even without Varjo/OpenXR in order to get FFR behavior.
        return pvr_success;
    }

    void emulate_shutdown() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_shutdown");

        if (varjoSession) {
            varjo_SessionShutDown(varjoSession);
            varjoSession = nullptr;
        }

        if (openXrInstance != XR_NULL_HANDLE) {
            // HACK: This will cause the OpenXR Thread to fail/exit.
            xrDestroyInstance(openXrInstance);
            openXrInstance = XR_NULL_HANDLE;
            openXrSession = XR_NULL_HANDLE;
            openXrEyeTrackerFB = XR_NULL_HANDLE;
            openXrViewSpace = XR_NULL_HANDLE;
            openXrThread.join();
        }

        Log("Terminated\n");

        TraceLoggingWriteStop(local, "PVR_shutdown");
    }

    pvrResult emulate_createHmd(pvrHmdHandle* phmdh) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_createHmd");

        // Initialize Varjo eye tracking.
        if (varjoSession) {
            varjo_GazeInit(varjoSession);
        } else if (openXrInstance != XR_NULL_HANDLE) {
            // Start the background thread.
            openXrThread = std::thread([]() { openXrEyeTrackerFBThread(); });
        }

        // Any fake handle.
        *phmdh = (pvrHmdHandle)0x1;

        TraceLoggingWriteStop(local, "PVR_createHmd");

        return pvr_success;
    }

    void emulate_destroyHmd(pvrHmdHandle hmdh) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_destroyHmd");
        TraceLoggingWriteStop(local, "PVR_destroyHmd");
    }

    pvrResult emulate_getEyeRenderInfo(pvrHmdHandle hmdh, pvrEyeType eye, pvrEyeRenderInfo* outInfo) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getEyeRenderInfo", TLArg((int)eye, "eye"));

        // Prepare a PVR struct from the Varjo information. It's unclear exactly which fields LibMagic actually needs,
        // so we just populate them all.

        double ipd = 0;
        if (varjoSession) {
            varjo_GetTextureSize(varjoSession,
                                 varjo_TextureSize_Type_Stereo,
                                 (uint32_t)eye,
                                 &outInfo->DistortedViewport.Size.w,
                                 &outInfo->DistortedViewport.Size.h);
            outInfo->DistortedViewport.Pos = {0, 0};

            const auto fov = varjo_GetFovTangents(varjoSession, (int)eye);
            outInfo->Fov.DownTan = (float)abs(fov.bottom);
            outInfo->Fov.LeftTan = (float)abs(fov.left);
            outInfo->Fov.RightTan = (float)abs(fov.right);
            outInfo->Fov.UpTan = (float)abs(fov.top);

            // Try auto-IPD, then manual IPD selection.
            ipd = varjo_GetPropertyDouble(varjoSession, varjo_PropertyKey_GazeIPDEstimate) / 1000;
            if (ipd < FLT_EPSILON) {
                ipd = varjo_GetPropertyDouble(varjoSession, varjo_PropertyKey_IPDPosition) / 1000;
            }

            static double loggedIpd = 0;
            if (abs(loggedIpd - ipd) > DBL_EPSILON) {
                Log("Detected IPD: %.4f\n", ipd);
            }
            loggedIpd = ipd;
#if 0
        } else if (openXrInstance != XR_NULL_HANDLE) {
            XrResult result = XR_ERROR_RUNTIME_FAILURE;

            XrViewConfigurationView views[2]{{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}};
            uint32_t count = 0;
            result = xrEnumerateViewConfigurationViews(
                openXrInstance, openXrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &count, views);
            outInfo->DistortedViewport.Size = {(int)views[0].recommendedImageRectWidth,
                                               (int)views[0].recommendedImageRectHeight};
            outInfo->DistortedViewport.Pos = {0, 0};

            xrLocateViews(openXrSession, &locateInfo, &viewState, 2, &count, views);
#endif
        } else {
            // Just some defaults...
            outInfo->DistortedViewport.Size = {2160, 2160};
            outInfo->DistortedViewport.Pos = {0, 0};
            outInfo->Fov.DownTan = outInfo->Fov.LeftTan = outInfo->Fov.RightTan = outInfo->Fov.UpTan =
                (float)tan(M_PI_4);
        }

        // Don't care? Just put a value that assumes uniform PPD.
        outInfo->PixelsPerTanAngleAtCenter.x =
            outInfo->DistortedViewport.Size.w / (abs(outInfo->Fov.LeftTan) + abs(outInfo->Fov.RightTan));
        outInfo->PixelsPerTanAngleAtCenter.y =
            outInfo->DistortedViewport.Size.h / (abs(outInfo->Fov.UpTan) + abs(outInfo->Fov.DownTan));

        // No canting.
        outInfo->HmdToEyePose.Orientation = {0, 0, 0, 1};

        if (ipd < FLT_EPSILON) {
            // Use a good fallback.
            ipd = 0.063;
        }
        outInfo->HmdToEyePose.Position = {((float)ipd / 2.f) * (eye == pvrEye_Left ? -1 : 1), 0, 0};

        TraceLoggingWriteStop(local, "PVR_getEyeRenderInfo");

        return pvr_success;
    }

    int emulate_getIntConfig(pvrHmdHandle hmdh, const char* key, int def_val) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getIntConfig", TLArg(key), TLArg(def_val));

#ifdef _DEBUG
        // Debug keys for experimenting.
        {
            static bool wasPressed = false;
            bool isFnPressed[13]{}; // 0=Any
            for (uint32_t i = 0; i < 12; i++) {
                isFnPressed[1 + i] = GetAsyncKeyState(VK_F1 + i) < 0;
                if (isFnPressed[1 + i]) {
                    isFnPressed[0] = true;
                }
            }
            const bool isPressed = GetAsyncKeyState(VK_CONTROL) < 0 && isFnPressed[0];
            if (isPressed && !wasPressed) {
                const uint32_t oldMode = mode.load();
                if (isFnPressed[1]) {
                    mode.store(0);
                } else if (isFnPressed[2]) {
                    updateMode();
                }

                if (isFnPressed[5]) {
                    mode.store(1);
                } else if (isFnPressed[6]) {
                    mode.store(2);
                } else if (isFnPressed[7]) {
                    mode.store(3);
                } else if (isFnPressed[8]) {
                    mode.store(4);
                }
            }
        }
#endif

        static uint32_t oldMode = -1;
        const uint32_t currentMode = mode.load();
        if (oldMode != currentMode) {
            if (!currentMode) {
                Log("Disabling foveation\n");
            } else if (currentMode == 4) {
                Log("Debug mode\n");
            } else {
                Log("Setting foveation level: %u\n", currentMode - 1);
            }
        }
        oldMode = currentMode;

        int value = def_val;
        {
            std::string_view strkey(key);
            if (strkey == "enable_foveated_rendering") {
                value = currentMode ? 1 : 0;
            } else if (strkey == "foveated_rendering_level") {
                value = currentMode - 1;
            }
        }

        TraceLoggingWriteStop(local, "PVR_getIntConfig", TLArg(value));

        return value;
    }

    pvrResult emulate_setIntConfig(pvrHmdHandle hmdh, const char* key, int val) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_setIntConfig", TLArg(key), TLArg(val));

        {
            std::string_view strkey(key);
            if (strkey == "foveated_rendering_active") {
                static bool wasActive = false;
                if (wasActive != !!val) {
                    Log("Foveation is %s\n", val ? "active" : "not active");
                }
                wasActive = !!val;
            }
        }

        TraceLoggingWriteStop(local, "PVR_setIntConfig");

        return pvr_success;
    }

    pvrResult emulate_getEyeTrackingInfo(pvrHmdHandle hmdh, double absTime, pvrEyeTrackingInfo* outInfo) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getEyeTrackingInfo", TLArg(absTime));

        const auto now = std::chrono::steady_clock::now();

        if (varjoSession || openXrInstance != XR_NULL_HANDLE) {
            // Clear old cache
            if ((now - lastGoodEyeTrackingDataTime).count() >= 600'000'000) {
                lastGoodEyeTrackingInfo.reset();
            }

            bool isValid = true;

            if (varjoSession) {
                // Query the most recent eye tracking data.
                varjo_Gaze gaze{};
                if (!ignoreEyeTracking.load()) {
                    gaze = varjo_GetGaze(varjoSession);
                }
                for (uint32_t i = 0; i < 2; i++) {
                    if ((i == 0 && gaze.leftStatus == varjo_GazeEyeStatus_Invalid) ||
                        (i == 1 && gaze.rightStatus == varjo_GazeEyeStatus_Invalid)) {
                        isValid = false;
                        break;
                    }

                    const auto& forward = i == 0 ? gaze.leftEye.forward : gaze.rightEye.forward;
                    const double r = sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
                    if (r < FLT_EPSILON) {
                        isValid = false;
                        break;
                    }

                    // This works well-enough.
                    outInfo->GazeTan[i] = {(float)forward[0], (float)forward[1]};
                }
            } else if (openXrInstance != XR_NULL_HANDLE) {
                std::unique_lock lock(openXrCacheMutex);

                isValid = openXrCacheGazeUnitVector.has_value();
                if (isValid) {
                    outInfo->GazeTan[0] = outInfo->GazeTan[1] = {openXrCacheGazeUnitVector.value().x,
                                                                 openXrCacheGazeUnitVector.value().y};
                }
            }

            if (isValid) {
                lastGoodEyeTrackingInfo = *outInfo;
                lastGoodEyeTrackingDataTime = now;
            } else if (!isValid && lastGoodEyeTrackingInfo) {
                // To avoid warping during blinking, we use a reasonably recent cached gaze vector.
                *outInfo = lastGoodEyeTrackingInfo.value();
                isValid = true;
            }

            outInfo->TimeInSeconds = isValid ? absTime : 0;
        } else {
            outInfo->TimeInSeconds = 0;
        }

        TraceLoggingWriteStop(local, "PVR_getEyeTrackingInfo");

        return pvr_success;
    }

    pvrInterface* emulate_getPvrInterface(uint32_t major_ver, uint32_t minor_ver) {
        static pvrInterface result;

        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_getInterface", TLArg(major_ver), TLArg(minor_ver));
        Log("Requested PVR SDK: %u.%u\n", major_ver, minor_ver);

        // We can only emulate the interface we were built against.
        if (major_ver != PVR_MAJOR_VERSION) {
            return nullptr;
        }

#ifdef _DEBUG
        // This block is used to track which functions must be provided. Use trial-error, forcing the program to
        // crash with an invalid jump at the fake address of the funtion.
        {
            uint64_t i = 0;
            uint64_t* p = (uint64_t*)&result;
            for (; p < (uint64_t*)(&result + 1); p++) {
                *p = ++i;
            }
        }
#endif

        // These are the functions that LibMagic seems to need.
        result.initialise = emulate_initialise;
        result.shutdown = emulate_shutdown;
        result.createHmd = emulate_createHmd;
        result.destroyHmd = emulate_destroyHmd;
        result.getEyeRenderInfo = emulate_getEyeRenderInfo;
        result.getIntConfig = emulate_getIntConfig;
        result.setIntConfig = emulate_setIntConfig;
        result.getEyeTrackingInfo = emulate_getEyeTrackingInfo;

        TraceLoggingWriteStop(local, "PVR_getInterface");

        return &result;
    }

} // namespace

// This is the entry point for libPVR.
extern "C" __declspec(dllexport) pvrInterface* getPvrInterface(uint32_t major_ver, uint32_t minor_ver) {
    return emulate_getPvrInterface(major_ver, minor_ver);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        TraceLoggingRegister(g_traceProvider);

        {
            const auto localAppData = std::filesystem::path(getenv("LOCALAPPDATA")) / "PvrEmu";
            CreateDirectoryA(localAppData.string().c_str(), nullptr);

            // Start logging to file.
            if (!logStream.is_open()) {
                std::string logFile = (localAppData / ("PvrEmu.log")).string();
                logStream.open(logFile, std::ios_base::ate);
            }

            char path[_MAX_PATH];
            GetModuleFileNameA(nullptr, path, sizeof(path));
            Log("Hello World from '%s'!\n", path);
        }

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
