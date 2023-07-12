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
        assert(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN, dll, &handle));

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        original = (TMethod)GetProcAddress(handle, target);
        assert(original);
        DetourAttach((PVOID*)&original, hooked);

        assert(DetourTransactionCommit() == NO_ERROR);
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

        assert(DetourTransactionCommit() == NO_ERROR);

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

    // Varjo session handle, most important things will go through that!
    varjo_Session* varjoSession = nullptr;

    pvrResult emulate_initialise() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_initialize");

        // Initialize the Varjo SDK.
        if (!varjoSession && varjo_IsAvailable()) {
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
        }

        TraceLoggingWriteStop(local, "PVR_initialize");

        // Succeed even without Varjo in order to get FFR behavior.
        return pvr_success;
    }

    void emulate_shutdown() {
        TraceLocalActivity(local);

        TraceLoggingWriteStart(local, "PVR_shutdown");

        if (varjoSession) {
            varjo_SessionShutDown(varjoSession);
            varjoSession = nullptr;
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

        static bool isEnabled = true; // Always enabled by default.
        static int level = 2;

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
                if (isFnPressed[1]) {
                    if (isEnabled) {
                        Log("Disabling foveation\n");
                    }
                    isEnabled = false;
                } else if (isFnPressed[2]) {
                    if (!isEnabled) {
                        Log("Enabling foveation\n");
                    }
                    isEnabled = true;
                }

                if (isFnPressed[5]) {
                    if (level != 0) {
                        Log("Setting foveation level: 0\n");
                    }
                    level = 0;
                } else if (isFnPressed[6]) {
                    if (level != 1) {
                        Log("Setting foveation level: 1\n");
                    }
                    level = 1;
                } else if (isFnPressed[7]) {
                    if (level != 2) {
                        Log("Setting foveation level: 2\n");
                    }
                    level = 2;
                } else if (isFnPressed[8]) {
                    if (level != 3) {
                        Log("Setting foveation level: 3\n");
                    }
                    level = 3;
                }
            }
        }
#endif

        int value = def_val;
        {
            std::string_view strkey(key);
            if (strkey == "enable_foveated_rendering") {
                value = isEnabled ? 1 : 0;
            } else if (strkey == "foveated_rendering_level") {
                value = level;
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

        if (varjoSession) {
            const auto gaze = varjo_GetGaze(varjoSession);
            bool isValid = true;
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
