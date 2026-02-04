using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Valve.VR;

namespace DFR_UI
{
    public partial class MainForm : Form
    {
        private uint ReadUInt32Setting(string name, uint defaultValue = 0u)
        {
            if (SettingsKey == null)
            {
                return defaultValue;
            }

            object val = SettingsKey.GetValue(name, defaultValue);
            if (val is uint ui) return ui;
            if (val is int i) return unchecked((uint)i);
            if (val is long l) return unchecked((uint)l);
            if (val is short s) return unchecked((uint)s);
            if (val is string str && uint.TryParse(str, out var parsed)) return parsed;

            try
            {
                // Fallback: attempt a conversion (handles other numeric types).
                return Convert.ToUInt32(val);
            }
            catch
            {
                return defaultValue;
            }
        }
        public MainForm()
        {
            InitializeComponent();
        }

        CVRSystem VrSystem;
        CVRApplications VrApplications;
        CVRCompositor VrCompositor;
        string PathToMagicAttach;
        uint AttachedApplication = 0;
        Microsoft.Win32.RegistryKey SettingsKey = Microsoft.Win32.Registry.CurrentUser.CreateSubKey("SOFTWARE\\FR-Utility");
        UInt32 AvailableEyeTrackers;
        bool[] AvailableEyeTrackersBool = new bool[6];
        private static readonly string[] EyeTrackerNames =
            {
                "HP Omnicept",          // 0
                "Virtual Desktop",    // 1
                "PSVR2 Toolkit",      // 2
                "Varjo",             // 3
                "Steam Link",         // 4
                "VRChat OSC"                // 5
            };

        

        private void MainForm_Load(object sender, EventArgs e)
        {
            SettingsKey.SetValue("force_eye_tracking_source", 0);
            this.Text += " v" + Assembly.GetExecutingAssembly().GetName().Version.ToString();

            var assembly = Assembly.GetAssembly(GetType());
            var installPath = Path.GetDirectoryName(assembly.Location);

            PathToMagicAttach = Path.Combine(installPath, "MagicAttach_x64.exe");

            var PathsToCheck = new string[] { PathToMagicAttach, Path.Combine(installPath, "LibMagicD3D1164.dll") };
            foreach (var path in PathsToCheck)
            {
                if (!File.Exists(path))
                {
                    MessageBox.Show(this, "Cannot find " + path + "\nPlease follow carefully the instructions for setting up this utility!", "Fatal Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Environment.Exit(1);
                }
            }

            var SystemFiles = new string[] {
                "libpvrclient64.dll",
                "jsoncpp.dll",
#if !DEBUG
                "libzmq-mt-4_3_3.dll",
#endif
                "VarjoLib.dll" };

            while (true)
            {
                var missingSystemFiles = "";
                foreach (var file in SystemFiles)
                {
                    var systemPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), file);

                    if (!File.Exists(systemPath) || !FileEquals(systemPath, Path.Combine(installPath, file)))
                    {
                        missingSystemFiles += systemPath + "\n";
                    }
                }

                if (missingSystemFiles.Length == 0)
                {
                    break;
                }

                var result = MessageBox.Show(this, "The following system files must be installed or updated in order to proceed.\n" + missingSystemFiles + "\n\nYou must close SteamVR before proceeding. Do you wish to continue?", "Installation", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result != DialogResult.Yes)
                {
                    Environment.Exit(1);
                }
                var processInfo = new ProcessStartInfo();
                processInfo.Verb = "RunAs";
                processInfo.FileName = "cmd.exe";
                processInfo.Arguments = "/s /c \"\"" + Path.Combine(installPath, "install_system_files.bat") + "\" \"" + installPath + "\"\"";
                processInfo.UseShellExecute = true;
                try
                {
                    var process = Process.Start(processInfo);
                    process.WaitForExit();
                    if (process.ExitCode != 0)
                    {
                        MessageBox.Show("Installation failed. Please make sure SteamVR is closed before trying again.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error, MessageBoxDefaultButton.Button1, MessageBoxOptions.DefaultDesktopOnly);
                        Environment.Exit(1);
                    }
                }
                catch (Win32Exception exc)
                {
                    MessageBox.Show("Failed to run the installation script: " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error, MessageBoxDefaultButton.Button1, MessageBoxOptions.DefaultDesktopOnly);
                    Environment.Exit(1);
                }

                // Force bring up the window.
                this.WindowState = FormWindowState.Minimized;
                this.Show();
                this.WindowState = FormWindowState.Normal;
            }

            switch ((int)SettingsKey.GetValue("mode", 0))
            {
                case 0:
                    frOff.Checked = true;
                    break;
                case 1:
                    frMaximum.Checked = true;
                    break;
                case 2:
                    frBalanced.Checked = true;
                    break;
                case 3:
                    frMinimum.Checked = true;
                    break;
                case 4:
                    frDebug.Checked = true;
                    break;
            }
            invertYAxis.Checked = (int)SettingsKey.GetValue("invert_y_axis", 0) == 0 ? false : true;
            forceFixed.Checked = (int)SettingsKey.GetValue("ignore_eye_tracking", 0) == 0 ? false : true;
            forceSource.Checked = (int)SettingsKey.GetValue("force_eye_tracking_source", 0) == 0 ? false : true;
            sourceList.SelectedItem = (string)SettingsKey.GetValue("eye_tracking_source_name", "");
        }

        void SetEnabled(bool enabled)
        {
            reattach.Enabled = labelMode.Enabled = frOff.Enabled = frMaximum.Enabled = frBalanced.Enabled =
                frMinimum.Enabled = frDebug.Enabled = invertYAxis.Enabled = forceFixed.Enabled = sourceList.Enabled = enabled;
            if (!enabled)
            {
                frameTimeLabel.Text = "";
            }
        }

        private void updateAvailableSources()
        {
            string saved = sourceList.GetItemText(sourceList.SelectedItem);

            sourceList.BeginUpdate(); // prevent flicker
            sourceList.Items.Clear();

            for (int i = 0; i < EyeTrackerNames.Length; i++)
            {
                if ((AvailableEyeTrackers & (1u << i)) != 0)
                {
                    sourceList.Items.Add(EyeTrackerNames[i]);
                }
            }

            // Restore selection only if possible
            if (sourceList.Items.Contains(saved))
            {
                sourceList.SelectedItem = saved;
            }
            else if (sourceList.Items.Count > 0)
            {
                sourceList.SelectedIndex = 0;
            }

            sourceList.EndUpdate();
        }


        private void timer1_Tick(object sender, EventArgs e)
        {
            AvailableEyeTrackers = ReadUInt32Setting("available_eye_trackers", 0u);
            updateAvailableSources();

            if (VrSystem == null || VrApplications == null || VrCompositor == null)
            {
                EVRInitError error = EVRInitError.Unknown;
                VrSystem = OpenVR.Init(ref error, EVRApplicationType.VRApplication_Background);
                if (VrSystem == null)
                {
                    appLabel.Text = "Not connected to SteamVR (" + error + ")";
                    SetEnabled(false);
                    return;
                }

                VrApplications = OpenVR.Applications;
                VrCompositor = OpenVR.Compositor;
            }

            if (VrSystem != null && VrApplications != null && VrCompositor != null)
            {
                uint pid = VrApplications.GetCurrentSceneProcessId();
                if (pid > 0)
                {
                    if (pid != AttachedApplication)
                    {
                        var sb = new StringBuilder((int)OpenVR.k_unMaxApplicationKeyLength);
                        VrApplications.GetApplicationKeyByProcessId(pid, sb, OpenVR.k_unMaxApplicationKeyLength);
                        var appKey = sb.ToString();
                        EVRApplicationError error = EVRApplicationError.None;
                        sb = new StringBuilder((int)OpenVR.k_unMaxPropertyStringSize);
                        VrApplications.GetApplicationPropertyString(appKey, EVRApplicationProperty.Name_String, sb, OpenVR.k_unMaxPropertyStringSize, ref error);
                        var appName = sb.ToString();
                        appLabel.Text = "Current application: " + appName + " (" + pid + ")";
                        SetEnabled(true);

                        MagicAttach(pid);
                        AttachedApplication = pid;
                    }
                }
                else
                {
                    appLabel.Text = "No SteamVR application is running";
                    SetEnabled(false);
                }

                var oldTextLength = log.TextLength;
                try
                {
                    var path = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\PvrEmu\\PvrEmu.log";
                    var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
                    var sr = new StreamReader(fs, Encoding.Default);
                    log.Text = sr.ReadToEnd();
                }
                catch (Exception exc)
                {
                    log.Text = "Failed to read log file: " + exc.Message;
                }
                if (oldTextLength != log.TextLength)
                {
                    log.SelectionStart = log.TextLength;
                    log.ScrollToCaret();
                }

                Compositor_FrameTiming timing = new Compositor_FrameTiming();
                timing.m_nSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf(typeof(Compositor_FrameTiming));
                if (VrCompositor.GetFrameTiming(ref timing, 0) && timing.m_flPreSubmitGpuMs > 0.0001f)
                {
                    frameTimeLabel.Text = "Application GPU frame time: " + (timing.m_flPreSubmitGpuMs + timing.m_flPostSubmitGpuMs).ToString("#.#") + "ms\n(for informational purposes only)";
                    frameTimeLabel.Location = new System.Drawing.Point(365, 108);
                }
            }
        }

        private void MagicAttach(uint pid)
        {
            var processInfo = new ProcessStartInfo();
            processInfo.Verb = "Run";
            processInfo.UseShellExecute = false;
            processInfo.CreateNoWindow = false;
            processInfo.FileName = PathToMagicAttach;
            processInfo.Arguments = "/pid " + pid;
            try
            {
                Process.Start(processInfo);
            }
            catch (Win32Exception)
            {
                MessageBox.Show("Failed to attach to process " + pid, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void frOff_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("mode", 0);
        }

        private void frMaximum_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("mode", 1);
        }

        private void frBalanced_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("mode", 2);
        }

        private void frMinimum_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("mode", 3);
        }

        private void frDebug_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("mode", 4);
        }

        private void reattach_Click(object sender, EventArgs e)
        {
            MagicAttach(AttachedApplication);
        }

        private void invertYAxis_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("invert_y_axis", invertYAxis.Checked ? 1 : 0);
        }

        private void forceFixed_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("ignore_eye_tracking", forceFixed.Checked ? 1 : 0);
        }

        private void forceSource_CheckedChanged(Object sender, EventArgs e)
        {
            SettingsKey.SetValue("force_eye_tracking_source", forceSource.Checked ? 1 : 0);
        }

        private void sourceList_SelectionChangeCommitted (object sender, EventArgs e)
        {
            forceSource.Enabled = !(sourceList.SelectedIndex == -1);
            SettingsKey.SetValue("eye_tracking_source_name", sourceList.GetItemText(sourceList.SelectedItem));
        }

        // https://stackoverflow.com/questions/968935/compare-binary-files-in-c-sharp
        static bool FileEquals(string fileName1, string fileName2)
        {
            using (var file1 = new FileStream(fileName1, FileMode.Open, FileAccess.Read))
            using (var file2 = new FileStream(fileName2, FileMode.Open, FileAccess.Read))
                return FileStreamEquals(file1, file2);
        }

        static bool FileStreamEquals(Stream stream1, Stream stream2)
        {
            const int bufferSize = 2048;
            byte[] buffer1 = new byte[bufferSize];
            byte[] buffer2 = new byte[bufferSize];
            while (true)
            {
                int count1 = stream1.Read(buffer1, 0, bufferSize);
                int count2 = stream2.Read(buffer2, 0, bufferSize);

                if (count1 != count2)
                    return false;

                if (count1 == 0)
                    return true;

                if (!buffer1.Take(count1).SequenceEqual(buffer2.Take(count2)))
                    return false;
            }
        }
    }
}
