// MIT License
//
// Copyright(c) 2022-2025 Matthieu Bucchianeri
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

#include <log.h>
#include <util.h>

#include "trackers.h"

namespace {

    using namespace logging;
    using namespace util;
    using namespace trackers;

    namespace {

        // Hook to fake the process ID.
        decltype(GetCurrentProcessId)* g_original_GetCurrentProcessId = nullptr;
        DWORD WINAPI hooked_GetCurrentProcessId() {
            // We try to only intercept calls from the Varjo client.
            HMODULE callerModule;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
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

    } // namespace

    struct VarjoEyeTracker : IEyeTracker {
        VarjoEyeTracker() {
            if (!varjo_IsAvailable()) {
                TraceLoggingWrite(g_traceProvider, "VarjoEyeTracker_NotAvailable");
                throw EyeTrackerNotSupportedException();
            }

            // We will fake process ID to make sure the Varjo SDK doesn't exit SteamVR.
            DetourDllAttach(
                "kernel32.dll", "GetCurrentProcessId", hooked_GetCurrentProcessId, g_original_GetCurrentProcessId);

            m_varjoSession = varjo_SessionInit();

            DetourDllDetach(
                "kernel32.dll", "GetCurrentProcessId", hooked_GetCurrentProcessId, g_original_GetCurrentProcessId);

            if (!m_varjoSession) {
                TraceLoggingWrite(g_traceProvider, "VarjoEyeTracker_InitError");
                throw EyeTrackerNotSupportedException();
            }
        }

        ~VarjoEyeTracker() override {
            varjo_SessionShutDown(m_varjoSession);
        }

        void start() override {
            varjo_GazeInit(m_varjoSession);
        }

        void stop() override {
        }

        bool isGazeAvailable(XrTime time) const override {
            const auto gaze = varjo_GetGaze(m_varjoSession);
            TraceLoggingWrite(g_traceProvider,
                              "VarjoEyeTracker_GetGaze",
                              TLArg((int)gaze.leftStatus, "LeftStatus"),
                              TLArg((int)gaze.rightStatus, "RightStatus"));

            if (gaze.leftStatus == varjo_GazeEyeStatus_Invalid || gaze.rightStatus == varjo_GazeEyeStatus_Invalid) {
                return false;
            }

            return true;
        }

        bool getGaze(XrTime time, XrVector3f& unitVector) override {
            const auto gaze = varjo_GetGaze(m_varjoSession);
            TraceLoggingWrite(g_traceProvider,
                              "VarjoEyeTracker_GetGaze",
                              TLArg((int)gaze.leftStatus, "LeftStatus"),
                              TLArg((int)gaze.rightStatus, "RightStatus"));

            if (gaze.leftStatus == varjo_GazeEyeStatus_Invalid || gaze.rightStatus == varjo_GazeEyeStatus_Invalid) {
                return false;
            }
            TraceLoggingWrite(g_traceProvider,
                              "VarjoEyeTracker_GetGaze",
                              TLArg(xr::ToString(XrVector3f{(float)gaze.leftEye.forward[0],
                                                            (float)gaze.leftEye.forward[1],
                                                            (float)gaze.leftEye.forward[2]})
                                        .c_str(),
                                    "LeftForward"),
                              TLArg(xr::ToString(XrVector3f{(float)gaze.rightEye.forward[0],
                                                            (float)gaze.rightEye.forward[1],
                                                            (float)gaze.rightEye.forward[2]})
                                        .c_str(),
                                    "RightForward"));

            unitVector.x = (float)(gaze.leftEye.forward[0] + gaze.rightEye.forward[0]) / 2.f;
            unitVector.y = (float)(gaze.leftEye.forward[1] + gaze.rightEye.forward[1]) / 2.f;
            unitVector.z = (float)(gaze.leftEye.forward[2] + gaze.rightEye.forward[2]) / 2.f;

            return true;
        }

        std::string getType() const override {
            return "Varjo";
        }

        varjo_Session* m_varjoSession{nullptr};
    };

} // namespace

namespace trackers {

    std::unique_ptr<IEyeTracker> createVarjoEyeTracker() {
        try {
            return std::make_unique<VarjoEyeTracker>();
        } catch (EyeTrackerNotSupportedException&) {
            return {};
        }
    }

} // namespace trackers
