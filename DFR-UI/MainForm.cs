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

namespace DFR_UI
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        string InstallPath;
        string[] DllsToInject64;
        string[] DllsToInject32;
        uint AttachedApplication = 0;
        Microsoft.Win32.RegistryKey SettingsKey;

        private void MainForm_Load(object sender, EventArgs e)
        {
            this.Text += " v" + Assembly.GetExecutingAssembly().GetName().Version.ToString();

            var assembly = Assembly.GetAssembly(GetType());
            var installPath = Path.GetDirectoryName(assembly.Location);
            InstallPath = installPath;

            // Loaded into the game in this order. LibMagic loads "libPVRClient{64,32}.dll" by name, which resolves to our
            // copy because it is already loaded in the process.
            DllsToInject64 = new string[] {
                Path.Combine(installPath, "libPVRClient64.dll"),
                Path.Combine(installPath, "LibMagicD3D1164.dll") };
            DllsToInject32 = new string[] {
                Path.Combine(installPath, "libPVRClient32.dll"),
                Path.Combine(installPath, "LibMagicD3D1132.dll") };

            foreach (var path in DllsToInject64)
            {
                if (!File.Exists(path))
                {
                    MessageBox.Show(this, "Cannot find " + path + "\nPlease follow carefully the instructions for setting up this utility!", "Fatal Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Environment.Exit(1);
                }
            }

            if (OpenXRLayer.NeedsRegistration(installPath))
            {
                var result = MessageBox.Show(this, "To use Foveated Rendering with OpenXR applications, the OpenXR API layer must be registered.\n\nDo you want to register it now?", "OpenXR support", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes && !OpenXRLayer.SetEnabledElevated(installPath, true))
                {
                    MessageBox.Show(this, "The OpenXR layer was not registered.", "OpenXR support", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

            SettingsKey = Microsoft.Win32.Registry.CurrentUser.CreateSubKey("SOFTWARE\\FR-Utility");
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

            ShowButtons(false);
            UpdateOpenXrLayerToggle();
        }

        void SetEnabled(bool enabled)
        {
            reattach.Enabled = labelMode.Enabled = frOff.Enabled = frMaximum.Enabled = frBalanced.Enabled =
                frMinimum.Enabled = frDebug.Enabled = invertYAxis.Enabled = forceFixed.Enabled = enabled;
            if (!enabled)
            {
                frameTimeLabel.Text = "";
            }
        }

        // Re-attaching only applies to OpenVR applications (OpenXR applications are handled by our API layer). Otherwise
        // show the OpenXR API layer toggle in the same spot.
        void ShowButtons(bool isOpenVrApp)
        {
            reattach.Visible = isOpenVrApp;
            openXrLayerToggle.Visible = !isOpenVrApp;
        }

        void UpdateOpenXrLayerToggle()
        {
            openXrLayerToggle.Text = OpenXRLayer.IsEnabled(InstallPath) ? "Disable OpenXR support" : "Enable OpenXR support";
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            // An OpenXR application (published by our API layer) works with any OpenXR runtime, and the layer loads
            // LibMagic itself: no injection needed. On SteamVR, it is also the current scene application, so record it as
            // attached to prevent the injection below.
            var openXrApp = OpenXRApp.GetCurrent();
            if (openXrApp != null)
            {
                appLabel.Text = "Current application: " + openXrApp.ApplicationName + " (" + openXrApp.ProcessId + ", OpenXR)";
                SetEnabled(true);
                ShowButtons(false);
                AttachedApplication = openXrApp.ProcessId;
            }

            // Otherwise, the current SteamVR scene application. SteamVR is not required, so it not running is not an error.
            var steamVrApp = SteamVRApp.GetCurrent();
            if (openXrApp != null)
            {
                // Already handled above.
            }
            else if (steamVrApp != null)
            {
                ShowButtons(true);
                if (steamVrApp.ProcessId != AttachedApplication)
                {
                    appLabel.Text = "Current application: " + steamVrApp.ApplicationName + " (" + steamVrApp.ProcessId + ", OpenVR)";
                    SetEnabled(true);

                    Attach(steamVrApp.ProcessId);
                    AttachedApplication = steamVrApp.ProcessId;
                }
            }
            else
            {
                appLabel.Text = "No VR application is running";
                SetEnabled(false);
                ShowButtons(false);
            }

            // The registration can also be changed by other tools.
            UpdateOpenXrLayerToggle();

            {
                var oldTextLength = log.TextLength;
                try
                {
                    var path = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\PimaxMagic4All\\PvrEmu.log";
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
            }

            var frameTime = SteamVRApp.GetApplicationGpuFrameTime();
            if (frameTime.HasValue)
            {
                frameTimeLabel.Text = "Application GPU frame time: " + frameTime.Value.ToString("#.#") + "ms\n(for informational purposes only)";
            }
        }

        private void Attach(uint pid)
        {
            try
            {
                var dlls = Injector.Is32BitProcess(pid) ? DllsToInject32 : DllsToInject64;
                foreach (var dll in dlls)
                {
                    if (!File.Exists(dll))
                    {
                        throw new FileNotFoundException("Cannot find " + dll);
                    }
                }
                foreach (var dll in dlls)
                {
                    Injector.Inject(pid, dll);
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("Failed to attach to process " + pid + ":\n" + exc.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void openXrLayerToggle_Click(object sender, EventArgs e)
        {
            // Takes effect for OpenXR applications started afterwards.
            var enable = !OpenXRLayer.IsEnabled(InstallPath);
            if (!OpenXRLayer.SetEnabledElevated(InstallPath, enable))
            {
                MessageBox.Show(this, "The OpenXR layer was not " + (enable ? "enabled" : "disabled") + ".", "OpenXR support", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            UpdateOpenXrLayerToggle();
        }

        private void reattach_Click(object sender, EventArgs e)
        {
            Attach(AttachedApplication);
        }

        private void invertYAxis_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("invert_y_axis", invertYAxis.Checked ? 1 : 0);
        }

        private void forceFixed_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("ignore_eye_tracking", forceFixed.Checked ? 1 : 0);
        }
    }
}
