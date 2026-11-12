using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DFR_UI
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static int Main(string[] args)
        {
            if (args.Length == 3 && args[0] == "/set-openxr-layer")
            {
                try
                {
                    OpenXRLayer.SetEnabled(args[1], args[2] == "enable");
                    return 0;
                }
                catch (Exception exc)
                {
                    MessageBox.Show("Failed to update the OpenXR layer: " + exc.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return 1;
                }
            }

            // Held for the lifetime of the app. The OpenXR layer only loads LibMagic while this mutex exists.
            using (var dfrUiRunning = new System.Threading.Mutex(false, "Local\\PimaxMagic4All-DFR-UI"))
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MainForm());
            }
            return 0;
        }
    }
}
