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

#include "openxr_layer.h"
#include "trackers.h"

namespace {

    using namespace logging;
    using namespace trackers;
    using namespace xr::math;

    // Eye tracking from the OpenXR runtime (XR_EXT_eye_gaze_interaction), queried by our API layer.
    struct OpenXrEyeTracker : IEyeTracker {
        OpenXrEyeTracker() {
            if (!openxr_layer::GetInstance().isEyeGazeSupported()) {
                TraceLoggingWrite(g_traceProvider, "OpenXrEyeTracker_NotSupported");
                throw EyeTrackerNotSupportedException();
            }
        }

        void start() override {
        }

        void stop() override {
        }

        bool getGaze(vr::HmdVector2_t& gaze) override {
            const auto eyeGaze = openxr_layer::GetInstance().getEyeGaze();
            if (!eyeGaze) {
                return false;
            }

            const auto gazePose = LoadXrPose(*eyeGaze);
            const auto gazeProjectedPoint = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.f, 0.f, -1.f, 1.f), gazePose);

            const auto unitVector = Normalize({DirectX::XMVectorGetX(gazeProjectedPoint),
                                               DirectX::XMVectorGetY(gazeProjectedPoint),
                                               DirectX::XMVectorGetZ(gazeProjectedPoint)});

            gaze.v[0] = unitVector.x;
            // TODO: Unclear why Y is flipped.
            gaze.v[1] = -unitVector.y;
            return true;
        }

        std::string getType() const override {
            return "OpenXR";
        }
    };

} // namespace

namespace trackers {

    std::unique_ptr<IEyeTracker> createOpenXrEyeTracker() {
        try {
            return std::make_unique<OpenXrEyeTracker>();
        } catch (EyeTrackerNotSupportedException&) {
            return {};
        }
    }

} // namespace trackers
