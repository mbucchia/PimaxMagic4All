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

using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace DFR_UI
{
    // Registers the OpenXR API layer manifests as implicit layers for the OpenXR loader.
    static class OpenXRLayer
    {
        const string ImplicitLayersKey = @"SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit";

        struct Manifest
        {
            public string Path;
            public RegistryView View;
        }

        static IEnumerable<Manifest> GetManifests(string installPath)
        {
            yield return new Manifest { Path = Path.Combine(installPath, "Magic4All.json"), View = RegistryView.Registry64 };
            yield return new Manifest { Path = Path.Combine(installPath, "Magic4All-32.json"), View = RegistryView.Registry32 };
        }

        public static bool NeedsRegistration(string installPath)
        {
            foreach (var manifest in GetManifests(installPath))
            {
                if (!File.Exists(manifest.Path))
                {
                    continue;
                }

                using (var baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, manifest.View))
                using (var key = baseKey.OpenSubKey(ImplicitLayersKey))
                {
                    if (key == null || key.GetValue(manifest.Path) == null)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        // Returns true if all our manifests are registered and enabled (value 0).
        public static bool IsEnabled(string installPath)
        {
            foreach (var manifest in GetManifests(installPath))
            {
                if (!File.Exists(manifest.Path))
                {
                    continue;
                }

                using (var baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, manifest.View))
                using (var key = baseKey.OpenSubKey(ImplicitLayersKey))
                {
                    var value = key?.GetValue(manifest.Path);
                    if (!(value is int) || (int)value != 0)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        // Registers our manifests (if needed) as enabled (value 0) or disabled (value 1). Requires administrator
        // privileges.
        public static void SetEnabled(string installPath, bool enabled)
        {
            foreach (var manifest in GetManifests(installPath))
            {
                if (!File.Exists(manifest.Path))
                {
                    continue;
                }

                using (var baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, manifest.View))
                using (var key = baseKey.CreateSubKey(ImplicitLayersKey, true))
                {
                    // Remove stale registrations of our manifest from a previous install location.
                    var fileName = Path.GetFileName(manifest.Path);
                    foreach (var name in key.GetValueNames())
                    {
                        if (string.Equals(Path.GetFileName(name), fileName, StringComparison.OrdinalIgnoreCase) &&
                            !string.Equals(name, manifest.Path, StringComparison.OrdinalIgnoreCase))
                        {
                            key.DeleteValue(name);
                        }
                    }

                    key.SetValue(manifest.Path, enabled ? 0 : 1, RegistryValueKind.DWord);
                }
            }
        }

        // Runs SetEnabled() in an elevated copy of this executable (UAC prompt). Returns false if the user declined the
        // elevation or the operation failed.
        public static bool SetEnabledElevated(string installPath, bool enabled)
        {
            var processInfo = new ProcessStartInfo();
            processInfo.Verb = "RunAs";
            processInfo.FileName = Assembly.GetExecutingAssembly().Location;
            processInfo.Arguments = "/set-openxr-layer \"" + installPath + "\" " + (enabled ? "enable" : "disable");
            processInfo.UseShellExecute = true;
            try
            {
                using (var process = Process.Start(processInfo))
                {
                    process.WaitForExit();
                    return process.ExitCode == 0;
                }
            }
            catch (Win32Exception)
            {
                return false;
            }
        }
    }
}
