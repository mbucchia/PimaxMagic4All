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

using System.Diagnostics;

namespace DFR_UI
{
    // Cumulative statistics of an application, as reported by our OpenXR API layer or by SteamVR.
    struct PerformanceSnapshot
    {
        public uint ProcessId;
        public uint FramesSubmitted;
        public double CpuFrameTimeSumMs;
        public uint CpuFrameTimeSamples;
        public double GpuFrameTimeSumMs;
        public uint GpuFrameTimeSamples;
    }

    // Averages over the interval between two consecutive snapshots.
    struct PerformanceAverages
    {
        public float? FrameRate;
        public float? CpuFrameTime; // ms
        public float? GpuFrameTime; // ms
    }

    // Derives averages from cumulative statistics sampled periodically.
    class PerformanceCounter
    {
        readonly Stopwatch Clock = Stopwatch.StartNew();
        PerformanceSnapshot? Last;
        double LastTime;

        // Returns the averages since the previous snapshot. Nothing is available on the first snapshot (or after a
        // reset or a change of application), and frame times are only available if frames were measured since.
        public PerformanceAverages Update(PerformanceSnapshot snapshot)
        {
            var now = Clock.Elapsed.TotalSeconds;
            var averages = new PerformanceAverages();
            if (Last.HasValue && Last.Value.ProcessId == snapshot.ProcessId && now > LastTime &&
                snapshot.FramesSubmitted >= Last.Value.FramesSubmitted)
            {
                var last = Last.Value;
                averages.FrameRate = (float)((snapshot.FramesSubmitted - last.FramesSubmitted) / (now - LastTime));
                averages.CpuFrameTime = Average(snapshot.CpuFrameTimeSumMs - last.CpuFrameTimeSumMs,
                                                (long)snapshot.CpuFrameTimeSamples - last.CpuFrameTimeSamples);
                averages.GpuFrameTime = Average(snapshot.GpuFrameTimeSumMs - last.GpuFrameTimeSumMs,
                                                (long)snapshot.GpuFrameTimeSamples - last.GpuFrameTimeSamples);
            }

            Last = snapshot;
            LastTime = now;
            return averages;
        }

        public void Reset()
        {
            Last = null;
        }

        static float? Average(double sum, long samples)
        {
            if (samples <= 0 || sum <= 0)
            {
                return null;
            }
            return (float)(sum / samples);
        }
    }
}
