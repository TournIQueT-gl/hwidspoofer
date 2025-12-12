using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Win32;

namespace HWIDChangerGUI
{
    public partial class MainForm : Form
    {
        private readonly HWIDService _service;
        private NotifyIcon _trayIcon;
        private bool _isClosing = false;
        private bool _isSpoofActive = false;
        private int _currentTab = 0;

        // Dragging
        private bool _isDragging = false;
        private Point _dragStart;

        [DllImport("Gdi32.dll")]
        private static extern IntPtr CreateRoundRectRgn(int x1, int y1, int x2, int y2, int cx, int cy);

        public MainForm()
        {
            try
            {
                InitializeComponent();
                _service = new HWIDService();
                _service.OutputReceived += OnOutput;
                _service.ErrorReceived += OnError;

                SetupWindow();
                SetupTray();
                Initialize();
                
                // 确保窗口可见
                this.Show();
                this.BringToFront();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Init error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void SetupWindow()
        {
            this.Region = System.Drawing.Region.FromHrgn(CreateRoundRectRgn(0, 0, Width, Height, Modern.Radius, Modern.Radius));
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            this.Region = System.Drawing.Region.FromHrgn(CreateRoundRectRgn(0, 0, Width, Height, Modern.Radius, Modern.Radius));
        }

        protected override CreateParams CreateParams
        {
            get { var cp = base.CreateParams; cp.ClassStyle |= 0x20000; return cp; }
        }

        private void SetupTray()
        {
            var menu = new ContextMenuStrip();
            menu.Items.Add("Open", null, (s, e) => { Show(); Activate(); });
            menu.Items.Add("-");
            menu.Items.Add("Quick Spoof", null, async (s, e) => await DoQuickSpoof());
            menu.Items.Add("Restore", null, async (s, e) => await DoRestore());
            menu.Items.Add("-");
            menu.Items.Add("Quit", null, (s, e) => { _isClosing = true; _trayIcon.Dispose(); Application.Exit(); });

            _trayIcon = new NotifyIcon { Text = "HWID Changer", Icon = SystemIcons.Shield, ContextMenuStrip = menu, Visible = true };
            _trayIcon.DoubleClick += (s, e) => { Show(); Activate(); };
        }

        private void Initialize()
        {
            if (!_service.IsExeAvailable)
            {
                SetStatus("Core not found", Color.FromArgb(255, 69, 58));
                btnMain.Enabled = false;
                Log("ERROR: HWIDChanger.exe not found", LT.Error);
                return;
            }

            if (_service.HasBackup())
            {
                _isSpoofActive = true;
                UpdateMainButton();
                SetStatus("Protected", Color.FromArgb(52, 199, 89));
                Log("Protection is active", LT.OK);
            }
            else
            {
                SetStatus("Not Protected", Color.FromArgb(142, 142, 147));
                Log("System ready", LT.Info);
            }

            LoadConfig();
            RefreshProcessList();
        }

        private void LoadConfig()
        {
            var cfg = _service.ReadConfigFile();
            if (string.IsNullOrEmpty(cfg)) return;

            foreach (var line in cfg.Split('\n'))
            {
                var t = line.Trim();
                if (t.StartsWith("#") || !t.Contains("=")) continue;
                var p = t.Split(new char[] { '=' }, 2);
                if (p.Length != 2) continue;
                var k = p[0].Trim().ToLower();
                var v = p[1].Trim();

                switch (k)
                {
                    case "disk_serial": txtDisk.Text = v; break;
                    case "volume_serial": txtVolume.Text = v; break;
                    case "machine_guid": txtGuid.Text = v; break;
                    case "product_id": txtProduct.Text = v; break;
                    case "mac_address": txtMac.Text = v; break;
                    case "bios_serial": txtBios.Text = v; break;
                }
            }
        }

        private string BuildConfig()
        {
            var sb = new System.Text.StringBuilder();
            sb.AppendLine("# HWID Config - " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
            if (!string.IsNullOrWhiteSpace(txtDisk.Text)) sb.AppendLine("disk_serial=" + txtDisk.Text);
            if (!string.IsNullOrWhiteSpace(txtVolume.Text)) sb.AppendLine("volume_serial=" + txtVolume.Text);
            if (!string.IsNullOrWhiteSpace(txtGuid.Text)) sb.AppendLine("machine_guid=" + txtGuid.Text);
            if (!string.IsNullOrWhiteSpace(txtProduct.Text)) sb.AppendLine("product_id=" + txtProduct.Text);
            if (!string.IsNullOrWhiteSpace(txtMac.Text)) sb.AppendLine("mac_address=" + txtMac.Text);
            if (!string.IsNullOrWhiteSpace(txtBios.Text)) sb.AppendLine("bios_serial=" + txtBios.Text);
            return sb.ToString();
        }

        // ============ WINDOW ============

        private void TitleMouseDown(object s, MouseEventArgs e) { if (e.Button == MouseButtons.Left) { _isDragging = true; _dragStart = e.Location; } }
        private void TitleMouseMove(object s, MouseEventArgs e) { if (_isDragging) { var p = PointToScreen(e.Location); Location = new Point(p.X - _dragStart.X, p.Y - _dragStart.Y); } }
        private void TitleMouseUp(object s, MouseEventArgs e) { _isDragging = false; }

        private void BtnClose_Click(object s, EventArgs e) { Hide(); _trayIcon.ShowBalloonTip(1000, "HWID Changer", "Running in tray", ToolTipIcon.Info); }
        private void BtnMin_Click(object s, EventArgs e) { WindowState = FormWindowState.Minimized; }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (!_isClosing && e.CloseReason == CloseReason.UserClosing) { e.Cancel = true; Hide(); }
            base.OnFormClosing(e);
        }

        // ============ TABS ============

        private void SwitchTab(int tab)
        {
            _currentTab = tab;
            panelQuick.Visible = tab == 0;
            panelAdvanced.Visible = tab == 1;
            panelConfig.Visible = tab == 2;

            navQuick.ForeColor = tab == 0 ? Modern.Primary : Modern.TextLight;
            navAdvanced.ForeColor = tab == 1 ? Modern.Primary : Modern.TextLight;
            navConfig.ForeColor = tab == 2 ? Modern.Primary : Modern.TextLight;

            navQuick.Font = new Font(navQuick.Font, tab == 0 ? FontStyle.Bold : FontStyle.Regular);
            navAdvanced.Font = new Font(navAdvanced.Font, tab == 1 ? FontStyle.Bold : FontStyle.Regular);
            navConfig.Font = new Font(navConfig.Font, tab == 2 ? FontStyle.Bold : FontStyle.Regular);
        }

        private void TabQuick_Click(object s, EventArgs e) { SwitchTab(0); }
        private void TabAdvanced_Click(object s, EventArgs e) { SwitchTab(1); }
        private void TabConfig_Click(object s, EventArgs e) { SwitchTab(2); }

        // ============ QUICK TAB ============

        private async void BtnMain_Click(object s, EventArgs e)
        {
            if (_isSpoofActive) await DoRestore();
            else await DoQuickSpoof();
        }

        private async void BtnGenerate_Click(object s, EventArgs e)
        {
            SetBusy(true);
            Log("Generating new IDs...", LT.Info);
            var r = await _service.GenerateNewConfig();
            if (r.Success) { await _service.SaveConfig(); LoadConfig(); Log("New IDs generated", LT.OK); }
            else Log("Generation failed", LT.Error);
            SetBusy(false);
        }

        private async Task DoQuickSpoof()
        {
            SetBusy(true);
            Log("Starting protection...", LT.Info);

            if (string.IsNullOrWhiteSpace(txtDisk.Text))
            {
                var gen = await _service.GenerateNewConfig();
                if (!gen.Success) { Log("Generation failed", LT.Error); SetBusy(false); return; }
                await _service.SaveConfig();
                LoadConfig();
            }

            Log("Applying to registry...", LT.Info);
            var r = await _service.ApplyRegistrySpoof();

            if (r.Success)
            {
                _isSpoofActive = true;
                UpdateMainButton();
                SetStatus("Protected", Color.FromArgb(52, 199, 89));
                Log("Protection enabled!", LT.OK);
            }
            else
            {
                Log("Failed: " + r.Error, LT.Error);
                SetStatus("Failed", Color.FromArgb(255, 69, 58));
            }
            SetBusy(false);
        }

        private async Task DoRestore()
        {
            SetBusy(true);
            Log("Restoring original...", LT.Info);
            var r = await _service.RestoreRegistry();

            if (r.Success)
            {
                _isSpoofActive = false;
                UpdateMainButton();
                SetStatus("Not Protected", Color.FromArgb(142, 142, 147));
                Log("Restored", LT.OK);
                txtDisk.Text = txtVolume.Text = txtGuid.Text = txtProduct.Text = txtMac.Text = txtBios.Text = "";
            }
            else Log("Restore failed: " + r.Error, LT.Error);
            SetBusy(false);
        }

        private void UpdateMainButton()
        {
            if (_isSpoofActive)
            {
                btnMain.Text = "Disable Protection";
                btnMain.ButtonColor = Modern.Danger;
            }
            else
            {
                btnMain.Text = "Enable Protection";
                btnMain.ButtonColor = Modern.Success;
            }
        }

        // ============ ADVANCED TAB ============

        private void RefreshProcessList()
        {
            cboProcess.Items.Clear();
            cboProcess.Items.Add("Select process...");
            try
            {
                foreach (var p in Process.GetProcesses())
                {
                    try { if (!string.IsNullOrEmpty(p.MainWindowTitle) || p.ProcessName.ToLower().Contains("game"))
                        cboProcess.Items.Add(string.Format("{0} ({1})", p.ProcessName, p.Id)); }
                    catch { }
                }
            }
            catch { }
            cboProcess.SelectedIndex = 0;
        }

        private void BtnRefresh_Click(object s, EventArgs e) { RefreshProcessList(); Log("Process list refreshed", LT.Info); }

        private async void BtnInject_Click(object s, EventArgs e)
        {
            if (cboProcess.SelectedIndex <= 0) { Log("Select a process first", LT.Warn); return; }

            var item = cboProcess.SelectedItem.ToString();
            var pidStart = item.LastIndexOf('(');
            var pidEnd = item.LastIndexOf(')');
            if (pidStart < 0 || pidEnd < 0) return;

            var pidStr = item.Substring(pidStart + 1, pidEnd - pidStart - 1);
            int pid;
            if (!int.TryParse(pidStr, out pid)) return;

            SetBusy(true);
            Log("Injecting to PID " + pid + "...", LT.Info);
            var r = await _service.InjectToProcess(pid);
            if (r.Success) Log("Injection successful!", LT.OK);
            else Log("Injection failed: " + r.Error, LT.Error);
            SetBusy(false);
        }

        private async void BtnAppInit_Click(object s, EventArgs e)
        {
            SetBusy(true);
            Log("Enabling AppInit (system-wide)...", LT.Info);
            var r = await _service.EnableAppInit();
            if (r.Success) Log("AppInit enabled - affects all new apps", LT.OK);
            else Log("Failed: " + r.Error, LT.Error);
            SetBusy(false);
        }

        private async void BtnAppInitOff_Click(object s, EventArgs e)
        {
            SetBusy(true);
            Log("Disabling AppInit...", LT.Info);
            var r = await _service.DisableAppInit();
            if (r.Success) Log("AppInit disabled", LT.OK);
            else Log("Failed: " + r.Error, LT.Error);
            SetBusy(false);
        }

        private async void BtnFullSpoof_Click(object s, EventArgs e)
        {
            SetBusy(true);
            Log("Applying FULL system spoof...", LT.Info);
            Log("  - Registry modification", LT.Info);
            Log("  - AppInit injection", LT.Info);
            var r = await _service.FullUsermodeSpoof();
            if (r.Success) { Log("Full spoof applied!", LT.OK); _isSpoofActive = true; UpdateMainButton(); SetStatus("Full Protection", Color.FromArgb(52, 199, 89)); }
            else Log("Failed: " + r.Error, LT.Error);
            SetBusy(false);
        }

        // ============ CONFIG TAB ============

        private void BtnSave_Click(object s, EventArgs e)
        {
            try
            {
                var path = Path.Combine(Application.StartupPath, "hwid_current.cfg");
                File.WriteAllText(path, BuildConfig());
                Log("Config saved", LT.OK);
            }
            catch (Exception ex) { Log("Save failed: " + ex.Message, LT.Error); }
        }

        private void BtnExport_Click(object s, EventArgs e)
        {
            using (var d = new SaveFileDialog { Filter = "Config|*.cfg", FileName = "hwid_export.cfg" })
            {
                if (d.ShowDialog() == DialogResult.OK)
                {
                    try { File.WriteAllText(d.FileName, BuildConfig()); Log("Exported: " + Path.GetFileName(d.FileName), LT.OK); }
                    catch { Log("Export failed", LT.Error); }
                }
            }
        }

        private void BtnImport_Click(object s, EventArgs e)
        {
            using (var d = new OpenFileDialog { Filter = "Config|*.cfg|All|*.*" })
            {
                if (d.ShowDialog() == DialogResult.OK)
                {
                    try
                    {
                        var content = File.ReadAllText(d.FileName);
                        foreach (var line in content.Split('\n'))
                        {
                            var t = line.Trim();
                            if (t.StartsWith("#") || !t.Contains("=")) continue;
                            var p = t.Split(new char[] { '=' }, 2);
                            if (p.Length != 2) continue;
                            var k = p[0].Trim().ToLower();
                            var v = p[1].Trim();
                            switch (k)
                            {
                                case "disk_serial": txtDisk.Text = v; break;
                                case "volume_serial": txtVolume.Text = v; break;
                                case "machine_guid": txtGuid.Text = v; break;
                                case "product_id": txtProduct.Text = v; break;
                                case "mac_address": txtMac.Text = v; break;
                                case "bios_serial": txtBios.Text = v; break;
                            }
                        }
                        Log("Imported: " + Path.GetFileName(d.FileName), LT.OK);
                    }
                    catch { Log("Import failed", LT.Error); }
                }
            }
        }

        private void BtnClear_Click(object s, EventArgs e)
        {
            txtDisk.Text = txtVolume.Text = txtGuid.Text = txtProduct.Text = txtMac.Text = txtBios.Text = "";
            Log("Fields cleared", LT.Info);
        }

        private void ChkAutoStart_Changed(object s, EventArgs e)
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Run", true))
                {
                    if (chkAutoStart.Checked) key.SetValue("HWIDChanger", "\"" + Application.ExecutablePath + "\" --minimized");
                    else key.DeleteValue("HWIDChanger", false);
                }
                Log("Auto-start " + (chkAutoStart.Checked ? "enabled" : "disabled"), LT.Info);
            }
            catch { }
        }

        // ============ UI ============

        private void SetBusy(bool busy)
        {
            btnMain.Enabled = !busy;
            btnGenerate.Enabled = !busy;
            progressBar.Visible = busy;
        }

        private void SetStatus(string text, Color color)
        {
            lblStatus.Text = text;
            lblStatus.ForeColor = Color.FromArgb(60, 60, 67);
            lblStatusIcon.ForeColor = color;
        }

        private enum LT { Info, OK, Warn, Error }

        private void Log(string msg, LT t)
        {
            Color c;
            switch (t)
            {
                case LT.OK: c = Color.FromArgb(52, 199, 89); break;
                case LT.Warn: c = Color.FromArgb(255, 159, 10); break;
                case LT.Error: c = Color.FromArgb(255, 69, 58); break;
                default: c = Color.FromArgb(142, 142, 147); break;
            }
            txtLog.SelectionColor = Color.FromArgb(174, 174, 178);
            txtLog.AppendText(DateTime.Now.ToString("HH:mm:ss "));
            txtLog.SelectionColor = c;
            txtLog.AppendText(msg + "\n");
            txtLog.ScrollToCaret();
        }

        private void OnOutput(string s) { if (InvokeRequired) BeginInvoke(new Action(() => Log(s, LT.Info))); else Log(s, LT.Info); }
        private void OnError(string s) { if (InvokeRequired) BeginInvoke(new Action(() => Log(s, LT.Error))); else Log(s, LT.Error); }
    }
}
