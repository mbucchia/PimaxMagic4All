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
        public MainForm()
        {
            InitializeComponent();
        }

        CVRSystem VrSystem;
        CVRApplications VrApplications;
        CVRCompositor VrCompositor;
        string[] DllsToInject64;
        string[] DllsToInject32;
        uint AttachedApplication = 0;
        Microsoft.Win32.RegistryKey SettingsKey;

        private void MainForm_Load(object sender, EventArgs e)
        {
            this.Text += " v" + Assembly.GetExecutingAssembly().GetName().Version.ToString();

            var assembly = Assembly.GetAssembly(GetType());
            var installPath = Path.GetDirectoryName(assembly.Location);

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

        private void timer1_Tick(object sender, EventArgs e)
        {
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

                        Attach(pid);
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

                Compositor_FrameTiming timing = new Compositor_FrameTiming();
                timing.m_nSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf(typeof(Compositor_FrameTiming));
                if (VrCompositor.GetFrameTiming(ref timing, 0) && timing.m_flPreSubmitGpuMs > 0.0001f)
                {
                    frameTimeLabel.Text = "Application GPU frame time: " + (timing.m_flPreSubmitGpuMs + timing.m_flPostSubmitGpuMs).ToString("#.#") + "ms\n(for informational purposes only)";
                }
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
