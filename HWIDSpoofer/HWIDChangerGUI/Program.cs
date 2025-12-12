using System;
using System.Linq;
using System.Threading;
using System.Windows.Forms;

namespace HWIDChangerGUI
{
    static class Program
    {
        private static Mutex mutex = null;

        [STAThread]
        static void Main(string[] args)
        {
            const string mutexName = "HWIDChangerGUI_SingleInstance";
            bool createdNew;

            mutex = new Mutex(true, mutexName, out createdNew);

            if (!createdNew)
            {
                MessageBox.Show("HWID Changer is already running!\nCheck system tray.", 
                    "Info", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            
            var form = new MainForm();
            
            // Start minimized if requested
            if (args.Contains("--minimized") || args.Contains("-m"))
            {
                form.WindowState = FormWindowState.Minimized;
                form.ShowInTaskbar = false;
                form.Visible = false;
            }
            
            Application.Run(form);

            GC.KeepAlive(mutex);
        }
    }
}
