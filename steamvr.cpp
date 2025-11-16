// MIT License
//
// Copyright(c) 2025 Matthieu Bucchianeri
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

    struct SteamVrEyeTracker : IEyeTracker {
        SteamVrEyeTracker() {
            const std::vector<const char*> RequestedExtensions = {
                //                XR_KHR_D3D11_ENABLE_EXTENSION_NAME,
                XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME,
                XR_MND_HEADLESS_EXTENSION_NAME,
            };
            const std::vector<XrViewConfigurationType> SupportedViewConfigurationTypes = {
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            };
            const std::vector<XrEnvironmentBlendMode> SupportedEnvironmentBlendModes = {
                XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
            };

            // Find the path to the SteamVR OpenXR runtime.
            wil::unique_hkey enumKey;
            const LSTATUS status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes", 0, KEY_READ, enumKey.put());

            DWORD maxLength = 0;
            RegQueryInfoKey(enumKey.get(),
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            &maxLength,
                            nullptr,
                            nullptr,
                            nullptr);
            maxLength++;

            DWORD index = 0;
            while (true) {
                std::wstring value(maxLength, 0);

                DWORD length = maxLength;
                if (RegEnumValue(enumKey.get(), index++, value.data(), &length, nullptr, nullptr, nullptr, nullptr)) {
                    break;
                }
                value.resize(length);

                if (EndsWith(value, L"\\steamxr_win64.json")) {
                    // Force XR_RUNTIME_JSON to SteamVR.
                    TraceLoggingWrite(
                        g_traceProvider, "SteamVrEyeTracker_UseRuntime", TLArg(value.c_str(), "JsonPath"));
                    SetEnvironmentVariable(L"XR_RUNTIME_JSON", value.c_str());
                    break;
                }
            }

            // Create instance and get the HMD system.
            xr::ExtensionContext extensions = xr::CreateExtensionContext(RequestedExtensions);
            if (!extensions.SupportsHeadless) {
                TraceLoggingWrite(
                    g_traceProvider, "SteamVrEyeTracker_CreateInstanceError", TLArg("NoHeadlessSupport", "Error"));
                throw EyeTrackerNotSupportedException();
            }
            if (!extensions.SupportsEyeGazeInteraction) {
                TraceLoggingWrite(g_traceProvider,
                                  "SteamVrEyeTracker_CreateInstanceError",
                                  TLArg("NoEyeGazeInteractionSupport", "Error"));
                throw EyeTrackerNotSupportedException();
            }

            XrInstanceCreateInfo instanceCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO};
            xr::SetEnabledExtensions(instanceCreateInfo, extensions.EnabledExtensions);
            xr::SetApplicationInfo(instanceCreateInfo.applicationInfo, {"PimaxMagic4All", 1}, {"", 1});

            XrResult result = xrCreateInstance(&instanceCreateInfo, m_instance.Put(xrDestroyInstance));
            if (XR_FAILED(result)) {
                TraceLoggingWrite(
                    g_traceProvider, "SteamVrEyeTracker_CreateInstanceError", TLArg(xr::ToCString(result), "Error"));
                throw EyeTrackerNotSupportedException();
            }

            XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
            CHECK_XRCMD(xrGetInstanceProperties(m_instance.Get(), &instanceProperties));

            std::optional<sample::SystemContext> system =
                sample::CreateSystemContext(m_instance.Get(),
                                            extensions,
                                            XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
                                            SupportedViewConfigurationTypes,
                                            SupportedEnvironmentBlendModes);
            if (!system) {
                TraceLoggingWrite(g_traceProvider, "SteamVrEyeTracker_NoSystem");
                throw EyeTrackerNotSupportedException();
            }

            m_system = std::move(*system);
            if (!m_system.EyeGazeInteractionProperties.supportsEyeGazeInteraction) {
                TraceLoggingWrite(g_traceProvider, "SteamVrEyeTracker_NoEyeGazeInteractionSupport");
                throw EyeTrackerNotSupportedException();
            }
        }

        ~SteamVrEyeTracker() override {
        }

        void start() override {
            // There seems to be a bug in SteamVR OpenXR and we still need to check requirements even when headless.
            //            XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
            //            PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR = nullptr;
            //            xrGetInstanceProcAddr(m_instance.Get(),
            //                                  "xrGetD3D11GraphicsRequirementsKHR",
            //                                  (PFN_xrVoidFunction*)&xrGetD3D11GraphicsRequirementsKHR);
            //            if (xrGetD3D11GraphicsRequirementsKHR) {
            //                xrGetD3D11GraphicsRequirementsKHR(m_instance.Get(), m_system.Id, &graphicsRequirements);
            //            }

            XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO, nullptr, 0, m_system.Id};
            CHECK_XRCMD(xrCreateSession(m_instance.Get(), &sessionCreateInfo, m_session.Put(xrDestroySession)));
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            CHECK_XRCMD(xrBeginSession(m_session.Get(), &sessionBeginInfo));
        }

        void stop() override {
            CHECK_XRCMD(xrEndSession(m_session.Get()));
            m_session.Reset();
        }

        bool isGazeAvailable(XrTime time) const override {
            return false;
        }

        bool getGaze(XrTime time, XrVector3f& unitVector) override {
            return false;
        }

        std::string getType() const override {
            return "OpenXR Headless (" + std::string(m_system.Properties.systemName) + ")";
        }

        xr::InstanceHandle m_instance;
        sample::SystemContext m_system;
        xr::SessionHandle m_session;
        std::string m_systemName;
    };

} // namespace

namespace trackers {

    std::unique_ptr<IEyeTracker> createSteamVrEyeTracker() {
        try {
            return std::make_unique<SteamVrEyeTracker>();
        } catch (EyeTrackerNotSupportedException&) {
            return {};
        }
    }

} // namespace trackers
