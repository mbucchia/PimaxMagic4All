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
        string PathToMagicAttach;
        uint AttachedApplication = 0;
        Microsoft.Win32.RegistryKey SettingsKey;

        private void MainForm_Load(object sender, EventArgs e)
        {
            var assembly = Assembly.GetAssembly(GetType());
            var installPath = Path.GetDirectoryName(assembly.Location);

            PathToMagicAttach = Path.Combine(installPath, "MagicAttach_x64.exe");

            var PathsToCheck = new string[] { PathToMagicAttach, Path.Combine(installPath, "LibMagicD3D1164.dll"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "libpvrclient64.dll"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "VarjoLib.dll")};
            foreach (var path in PathsToCheck)
            {
                if (!File.Exists(path))
                {
                    MessageBox.Show(this, "Cannot find " + path + "\nPlease follow carefully the instructions for setting up this utility!", "Fatal Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Application.Exit();
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
            forceFixed.Checked = (int)SettingsKey.GetValue("ignore_eye_tracking", 0) == 0 ? false : true;
        }

        void SetEnabled(bool enabled)
        {
            reattach.Enabled = labelMode.Enabled = frOff.Enabled = frMaximum.Enabled = frBalanced.Enabled =
                frMinimum.Enabled = frDebug.Enabled = forceFixed.Enabled = enabled;
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (VrSystem == null)
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
            }

            if (VrSystem != null)
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
                    var fs = new FileStream(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) + "\\PvrEmu\\PvrEmu.log", FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
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

        private void forceFixed_CheckedChanged(object sender, EventArgs e)
        {
            SettingsKey.SetValue("ignore_eye_tracking", forceFixed.Checked ? 1 : 0);
        }
    }
}
