// MIT License
//
// Copyright(c) 2024-2026 Matthieu Bucchianeri
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

using System.Runtime.InteropServices;
using System.Text;
using Valve.VR;

namespace DFR_UI
{
    // Queries the current SteamVR scene application (OpenVR, or OpenXR on the SteamVR runtime).
    static class SteamVRApp
    {
        static CVRSystem VrSystem;
        static CVRApplications VrApplications;
        static CVRCompositor VrCompositor;

        public class Info
        {
            public uint ProcessId;
            public string ApplicationName;
        }

        // Connects to SteamVR as a background application if not done yet. Returns false if SteamVR is not running.
        static bool Connect()
        {
            if (VrSystem == null || VrApplications == null || VrCompositor == null)
            {
                EVRInitError error = EVRInitError.None;
                VrSystem = OpenVR.Init(ref error, EVRApplicationType.VRApplication_Background);
                if (VrSystem != null)
                {
                    VrApplications = OpenVR.Applications;
                    VrCompositor = OpenVR.Compositor;
                }
            }
            return VrSystem != null && VrApplications != null && VrCompositor != null;
        }

        // Returns the current scene application, or null if SteamVR or no application is running.
        public static Info GetCurrent()
        {
            if (!Connect())
            {
                return null;
            }

            uint processId = VrApplications.GetCurrentSceneProcessId();
            if (processId == 0)
            {
                return null;
            }

            var sb = new StringBuilder((int)OpenVR.k_unMaxApplicationKeyLength);
            VrApplications.GetApplicationKeyByProcessId(processId, sb, OpenVR.k_unMaxApplicationKeyLength);
            var appKey = sb.ToString();
            EVRApplicationError error = EVRApplicationError.None;
            sb = new StringBuilder((int)OpenVR.k_unMaxPropertyStringSize);
            VrApplications.GetApplicationPropertyString(appKey, EVRApplicationProperty.Name_String, sb, OpenVR.k_unMaxPropertyStringSize, ref error);

            return new Info
            {
                ProcessId = processId,
                ApplicationName = sb.ToString()
            };
        }

        // Returns the cumulative statistics of the current scene application, or null if not available. Does not try
        // to connect (GetCurrent() does).
        public static PerformanceSnapshot? GetApplicationPerformance(uint processId)
        {
            if (VrCompositor == null)
            {
                return null;
            }

            Compositor_CumulativeStats stats = new Compositor_CumulativeStats();
            VrCompositor.GetCumulativeStats(ref stats, (uint)Marshal.SizeOf(typeof(Compositor_CumulativeStats)));
            if (stats.m_nPid != processId)
            {
                return null;
            }

            // SteamVR sums the frame times over all submitted frames.
            return new PerformanceSnapshot
            {
                ProcessId = processId,
                FramesSubmitted = stats.m_nNumFrameSubmits,
                CpuFrameTimeSumMs = stats.m_flSumApplicationCPUTimeMS,
                CpuFrameTimeSamples = stats.m_nNumFrameSubmits,
                GpuFrameTimeSumMs = stats.m_flSumApplicationGPUTimeMS,
                GpuFrameTimeSamples = stats.m_nNumFrameSubmits
            };
        }
    }
}
