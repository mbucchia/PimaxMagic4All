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

using System;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Text;

namespace DFR_UI
{
    // Reads the OpenXR application published by our API layer (see SharedApplicationInfo in Magic4All/openxr_layer.cpp).
    static class OpenXRApp
    {
        const string MappingName = "Local\\PimaxMagic4All-OpenXR-App";
        const uint Version = 1;
        const int ApplicationNameSize = 128; // XR_MAX_APPLICATION_NAME_SIZE

        // Layout: uint32 version, uint32 processId (0 while not valid), char applicationName[128] (UTF-8), then
        // cumulative statistics: uint64 cpuFrameTimeSumUs, uint64 gpuFrameTimeSumUs, uint32 cpuFrameTimeSamples,
        // uint32 gpuFrameTimeSamples, uint32 framesSubmitted, uint32 reserved.
        const long VersionOffset = 0;
        const long ProcessIdOffset = 4;
        const long ApplicationNameOffset = 8;
        const long CpuFrameTimeSumOffset = ApplicationNameOffset + ApplicationNameSize;
        const long GpuFrameTimeSumOffset = CpuFrameTimeSumOffset + 8;
        const long CpuFrameTimeSamplesOffset = GpuFrameTimeSumOffset + 8;
        const long GpuFrameTimeSamplesOffset = CpuFrameTimeSamplesOffset + 4;
        const long FramesSubmittedOffset = GpuFrameTimeSamplesOffset + 4;
        const long Size = FramesSubmittedOffset + 8;

        public class Info
        {
            public uint ProcessId;
            public string ApplicationName;
            public PerformanceSnapshot Performance;
        }

        // Returns the running OpenXR application, or null. The mapping is opened and closed on every call, so that it
        // goes away when the application exits.
        public static Info GetCurrent()
        {
            try
            {
                using (var mapping = MemoryMappedFile.OpenExisting(MappingName, MemoryMappedFileRights.Read))
                using (var view = mapping.CreateViewAccessor(0, Size, MemoryMappedFileAccess.Read))
                {
                    if (view.ReadUInt32(VersionOffset) != Version)
                    {
                        return null;
                    }

                    var processId = view.ReadUInt32(ProcessIdOffset);
                    if (processId == 0)
                    {
                        return null;
                    }

                    var name = new byte[ApplicationNameSize];
                    view.ReadArray(ApplicationNameOffset, name, 0, name.Length);
                    var length = Array.IndexOf(name, (byte)0);

                    // The mapping can outlive the application if another OpenXR application still holds it.
                    using (Process.GetProcessById((int)processId))
                    {
                    }

                    // The layer updates each sum before its sample count, so read the counts first.
                    var performance = new PerformanceSnapshot
                    {
                        ProcessId = processId,
                        FramesSubmitted = view.ReadUInt32(FramesSubmittedOffset),
                        CpuFrameTimeSamples = view.ReadUInt32(CpuFrameTimeSamplesOffset),
                        GpuFrameTimeSamples = view.ReadUInt32(GpuFrameTimeSamplesOffset),
                    };
                    performance.CpuFrameTimeSumMs = view.ReadUInt64(CpuFrameTimeSumOffset) / 1000.0;
                    performance.GpuFrameTimeSumMs = view.ReadUInt64(GpuFrameTimeSumOffset) / 1000.0;

                    return new Info
                    {
                        ProcessId = processId,
                        ApplicationName = Encoding.UTF8.GetString(name, 0, length >= 0 ? length : name.Length),
                        Performance = performance
                    };
                }
            }
            catch (FileNotFoundException)
            {
                // No OpenXR application is running.
                return null;
            }
            catch (ArgumentException)
            {
                // The process that published is gone.
                return null;
            }
            catch (Exception)
            {
                return null;
            }
        }
    }
}
