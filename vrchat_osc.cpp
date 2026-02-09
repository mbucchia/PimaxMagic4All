// MIT License
//
// Copyright(c) 2022-2026 Matthieu Bucchianeri
// Copyright(c) 2025 Tymon Lindell (Ridge)
// Copyright(c) 2026 Bevergames2018
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

#include <osc/OscReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>

namespace {

    using namespace logging;
    using namespace trackers;

    struct VRChatOSCEyeTracker : IEyeTracker, osc::OscPacketListener {
        // VRChat's packets run over port 9000. This can be set to other ports if the software supports, we're using
        // port 9020 here.
        VRChatOSCEyeTracker() {
            wil::unique_handle mutex;
            *mutex.put() = OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\baballonia-unique-id");
            if (!mutex) {
                TraceLoggingWrite(g_traceProvider, "VRChatOSCEyeTracker_NoBaballoniaService");
                throw EyeTrackerNotSupportedException();
            }

            m_socket =
                std::make_unique<UdpListeningReceiveSocket>(IpEndpointName(IpEndpointName::ANY_ADDRESS, 9020), this);
        }

        ~VRChatOSCEyeTracker() override {
            if (m_started) {
                m_socket->AsynchronousBreak();
                m_listeningThread.join();
            }
        }

        void start() override {
            m_listeningThread = std::thread([&]() { m_socket->Run(); });
            m_started = true;
        }

        void stop() override {
        }

        bool getGaze(vr::HmdVector2_t& gaze) override {
            std::unique_lock lock(m_mutex);

            const auto now = std::chrono::high_resolution_clock::now();
            if ((now - m_lastReceivedTime).count() > 1'000'000'000) {
                return false;
            }

            gaze.v[0] = m_latestGaze.v[0];
            gaze.v[1] = m_latestGaze.v[1];
            return true;
        }

        std::string getType() const override {
            return "VRChat OSC";
        }

        void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) override {
            try {
                if (std::string_view(m.AddressPattern()) == "/tracking/eye/LeftRightPitchYaw") {
                    const auto now = std::chrono::high_resolution_clock::now();

                    osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

                    float leftPitch;
                    float leftYaw;
                    float rightPitch;
                    float rightYaw;
                    args >> leftPitch >> leftYaw >> rightPitch >> rightYaw >> osc::EndMessage;

                    // Convert degrees to radians for trigonometric functions
                    // Need to invert pitch because that's what mbucchia's code wants
                    const float leftPitchRad = leftPitch * (float)M_PI / 180.0f * -1.0f;
                    const float leftYawRad = leftYaw * (float)M_PI / 180.0f;
                    const float rightPitchRad = rightPitch * (float)M_PI / 180.0f * -1.0f;
                    const float rightYawRad = rightYaw * (float)M_PI / 180.0f;

                    vr::HmdVector3_t unitVector = {
                        (sin(leftYawRad) * cos(leftPitchRad) + sin(rightYawRad) * cos(rightPitchRad)) / 2,
                        (sin(leftPitchRad) + sin(rightPitchRad)) / 2,
                        (-cos(leftYawRad) * cos(leftPitchRad) - cos(rightYawRad) * cos(rightPitchRad)) / 2};

                    TraceLoggingWrite(g_traceProvider,
                                      "VRChatOSCEyeTracker_ProcessMessage",
                                      TLArg(leftPitch, "LeftPitch"),
                                      TLArg(leftYaw, "LeftYaw"),
                                      TLArg(rightPitch, "RightPitch"),
                                      TLArg(rightYaw, "RightYaw"));

                    if (!(std::isnan(leftPitch) || std::isnan(leftYaw) || std::isnan(rightPitch) ||
                          std::isnan(rightYaw))) {
                        std::unique_lock lock(m_mutex);
                        m_latestGaze = unitVector;
                        m_lastReceivedTime = now;
                    }
                }
            } catch (osc::Exception& e) {
                TraceLoggingWrite(g_traceProvider, "VRChatOSCEyeTracker_ProcessMessage", TLArg(e.what(), "Error"));
            }
        }

        bool m_started{false};
        std::thread m_listeningThread;
        std::unique_ptr<UdpListeningReceiveSocket> m_socket;
        mutable std::mutex m_mutex;
        vr::HmdVector3_t m_latestGaze{};
        std::chrono::high_resolution_clock::time_point m_lastReceivedTime{};
    };

} // namespace

namespace trackers {

    std::unique_ptr<IEyeTracker> createVRChatOSCEyeTracker() {
        try {
            return std::make_unique<VRChatOSCEyeTracker>();
        } catch (EyeTrackerNotSupportedException&) {
            return {};
        }
    }

} // namespace trackers
