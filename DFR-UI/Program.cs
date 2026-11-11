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
            if (args.Length == 2 && args[0] == "/register-openxr-layer")
            {
                try
                {
                    OpenXRLayer.Register(args[1]);
                    return 0;
                }
                catch (Exception exc)
                {
                    MessageBox.Show("Failed to register the OpenXR layer: " + exc.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return 1;
                }
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
            return 0;
        }
    }
}
