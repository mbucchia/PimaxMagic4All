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

#include "log.h"
using namespace openxr_api_layer::log;

#include <trackers.h>
using namespace openxr_api_layer;

//
// Log file helpers.
//

namespace openxr_api_layer::log {
    // {cbf3adcd-42b9-4c38-830b-91980af201f6}
    TRACELOGGING_DEFINE_PROVIDER(g_traceProvider,
                                 "PvrEmu",
                                 (0xcbf3adcd, 0x42b9, 0x4c38, 0x83, 0x0b, 0x91, 0x98, 0x0a, 0xf2, 0x01, 0xf6));

    TraceLoggingActivity<g_traceProvider> g_traceActivity;

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

} // namespace openxr_api_layer::log

//
// And now, the real stuff.
//

namespace {

    std::unique_ptr<IEyeTracker> eyeTracker;

    wil::unique_registry_watcher registryWatcher;
    std::atomic<uint32_t> mode = 0;
    std::atomic<bool> ignoreEyeTracking = 0;

    std::chrono::time_point<std::chrono::steady_clock> lastGoodEyeTrackingDataTime{};
    std::optional<pvrEyeTrackingInfo> lastGoodEyeTrackingInfo;

    vr::IVRSystem* openvrSystem = nullptr;

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

    pvrResult emulate_initialise() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_initialize");

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

        // Retrieve the IVRSystem. If we are in this function now, then it means someone initialized it at some point.
        HMODULE openvr = nullptr;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"openvr_api.dll", &openvr);
        if (openvr) {
            void* (*pfnVR_GetGenericInterface)(const char* pchInterfaceVersion, vr::EVRInitError* peError) =
                (decltype(pfnVR_GetGenericInterface))GetProcAddress(openvr, "VR_GetGenericInterface");
            if (pfnVR_GetGenericInterface) {
                vr::EVRInitError error;
                openvrSystem = (vr::IVRSystem*)pfnVR_GetGenericInterface("IVRSystem_022", &error);
            }
        }

        if (!openvrSystem) {
            Log("Unable to retrieve IVRSystem, projection may be inaccurate\n");
        }

        // Initialize the eye tracker. We try in order from "strongest check" to "weakest check".
        std::function<std::unique_ptr<IEyeTracker>()> eyeTrackers[] = {
            // 1) Omnicept uses a background service, it is not likely to be installed if the device is not used.
            createOmniceptEyeTracker,
            // 2) Virtual Desktop driver for SteamVR shall only be loaded if the streamer app is opened.
            createVirtualDesktopEyeTracker,
            // 3) Varjo only loads if Varjo Base is running.
            createVarjoEyeTracker,
            // 4) Steam Link doesn't have any check.
            createSteamLinkEyeTracker,
        };
        for (uint32_t i = 0; !eyeTracker && i < std::size(eyeTrackers); i++) {
            eyeTracker = eyeTrackers[i]();
        }

        if (eyeTracker) {
            TraceLoggingWrite(
                g_traceProvider, "EyeTracker", TLArg(getTrackerType(eyeTracker->getType()).c_str(), "Type"));
            Log(fmt::format("Using eye tracking: {}\n", getTrackerType(eyeTracker->getType())));
        } else {
            Log("No supported eye tracking device found\n");
        }

        // Initial reading of the settings.
        updateMode();

        TraceLoggingWriteStop(local, "PVR_initialize");

        // Succeed even without an eye tracker in order to get FFR behavior.
        return pvr_success;
    }

    void emulate_shutdown() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_shutdown");

        eyeTracker.reset();

        Log("Terminated\n");

        TraceLoggingWriteStop(local, "PVR_shutdown");
    }

    pvrResult emulate_createHmd(pvrHmdHandle* phmdh) {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_createHmd");

        // Initialize eye tracking.
        if (eyeTracker) {
            eyeTracker->start(XR_NULL_HANDLE);
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

        // It's unclear exactly which fields LibMagic actually needs, so we just populate them all.
        // Refine parameters if we can. This will produce a better outcome (proper eye convergence).
        if (openvrSystem) {
            float bottom, left, right, top;
            // Note that top and bottom are swapped (empirical mistake in OpenVR?).
            openvrSystem->GetProjectionRaw((vr::EVREye)eye, &left, &right, &bottom, &top);
            outInfo->Fov.DownTan = (float)abs(bottom);
            outInfo->Fov.LeftTan = (float)abs(left);
            outInfo->Fov.RightTan = (float)abs(right);
            outInfo->Fov.UpTan = (float)abs(top);
        } else {
            outInfo->Fov.DownTan = outInfo->Fov.LeftTan = outInfo->Fov.RightTan = outInfo->Fov.UpTan =
                (float)tan(M_PI_4);
        }

        // Don't care? These values seem to make no difference.
        outInfo->DistortedViewport.Pos = {0, 0};
        outInfo->DistortedViewport.Size = {2160, 2160};

        // Don't care? Just put a value that assumes uniform PPD.
        outInfo->PixelsPerTanAngleAtCenter.x =
            outInfo->DistortedViewport.Size.w / (abs(outInfo->Fov.LeftTan) + abs(outInfo->Fov.RightTan));
        outInfo->PixelsPerTanAngleAtCenter.y =
            outInfo->DistortedViewport.Size.h / (abs(outInfo->Fov.UpTan) + abs(outInfo->Fov.DownTan));

        // No canting.
        const float ipd = 0.063f;
        outInfo->HmdToEyePose.Position = {(ipd / 2.f) * (eye == pvrEye_Left ? -1 : 1), 0, 0};
        outInfo->HmdToEyePose.Orientation = {0, 0, 0, 1};

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

        if (eyeTracker) {
            const auto now = std::chrono::steady_clock::now();

            // Clear old cache
            if ((now - lastGoodEyeTrackingDataTime).count() >= 600'000'000) {
                lastGoodEyeTrackingInfo.reset();
            }

            // Query the most recent eye tracking data.
            XrVector3f gaze{};
            bool isValid = false;
            if (!ignoreEyeTracking.load()) {
                isValid = eyeTracker->getGaze(0, gaze);
            }
            for (uint32_t i = 0; i < 2; i++) {
                // Our gaze vector is normalized.
                // This works well-enough.
                outInfo->GazeTan[i] = {gaze.x, gaze.y};
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
