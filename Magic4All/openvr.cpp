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

#include <log.h>
#include <util.h>

#include "trackers.h"

namespace {

    using namespace logging;
    using namespace trackers;

    struct OpenVrEyeTracker : IEyeTracker {
        OpenVrEyeTracker(vr::IVRSystem* system) {
            if (!system->GetBoolTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd,
                                                      vr::Prop_SupportsXrEyeGazeInteraction_Bool)) {
                TraceLoggingWrite(g_traceProvider, "OpenVrEyeTracker_NotSupportsXrEyeGazeInteraction");
                throw EyeTrackerNotSupportedException();
            }

            m_system = system;

            char systemName[256]{};
            m_system->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd,
                                                     vr::Prop_ActualTrackingSystemName_String,
                                                     systemName,
                                                     sizeof(systemName));

            char model[256]{};
            m_system->GetStringTrackedDeviceProperty(
                vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String, model, sizeof(model));
            m_systemName = systemName;
            if (model[0]) {
                m_systemName += "/" + std::string(model);
            }
        }

        ~OpenVrEyeTracker() override {
        }

        void start() override {
        }

        void stop() override {
        }

        bool getGaze(vr::HmdVector2_t& gaze) override {
            vr::HmdVector2_t leftNdc{}, rightNdc{};
            if (!m_system->GetEyeTrackedFoveationCenter(&leftNdc, &rightNdc)) {
                return false;
            }

            gaze.v[0] = (leftNdc.v[0] + rightNdc.v[0]) / 2.f;
            gaze.v[1] = (leftNdc.v[1] + rightNdc.v[1]) / 2.f;
            return true;
        }

        std::string getType() const override {
            return "OpenVR (" + m_systemName + ")";
        }

        vr::IVRSystem* m_system{nullptr};
        std::string m_systemName;
    };

} // namespace

namespace trackers {

    std::unique_ptr<IEyeTracker> createOpenVrEyeTracker(vr::IVRSystem* system) {
        try {
            return std::make_unique<OpenVrEyeTracker>(system);
        } catch (EyeTrackerNotSupportedException&) {
            return {};
        }
    }

} // namespace trackers
