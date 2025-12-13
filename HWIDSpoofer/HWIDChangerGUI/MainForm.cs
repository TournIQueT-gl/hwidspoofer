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
        private ToolTip _toolTip;
        private bool _isClosing = false;
        private bool _isSpoofActive = false;
        private int _currentTab = 0;
        private Timer _welcomeTimer;

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
            this.Region = System.Drawing.Region.FromHrgn(CreateRoundRectRgn(0, 0, Width, Height, 16, 16));
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            this.Region = System.Drawing.Region.FromHrgn(CreateRoundRectRgn(0, 0, Width, Height, 16, 16));
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
            // Setup tooltips for better UX
            SetupTooltips();
            
            // Setup keyboard shortcuts
            this.KeyPreview = true;
            this.KeyDown += OnKeyDown;

            if (!_service.IsExeAvailable)
            {
                UpdateStatusCard("Core Not Found", "HWIDChanger.exe is missing", Theme.Danger, "!");
                btnMain.Enabled = false;
                Log("HWIDChanger.exe not found", LT.Error);
                ShowNotification("Error", "Core component not found", ToolTipIcon.Error);
                return;
            }

            if (_service.HasBackup())
            {
                _isSpoofActive = true;
                UpdateMainButton();
                UpdateStatusCard("Protected", "Your hardware IDs are spoofed", Theme.Success, "✓");
                Log("Protection active", LT.OK);
            }
            else
            {
                UpdateStatusCard("Not Protected", "Click below to enable protection", Theme.TextMuted, "○");
                // Welcome message with delay
                _welcomeTimer = new Timer { Interval = 500 };
                _welcomeTimer.Tick += (s, e) => {
                    _welcomeTimer.Stop();
                    Log("Welcome! Press F1 for help", LT.Info);
                };
                _welcomeTimer.Start();
            }

            LoadConfig();
            RefreshProcessList();
        }

        private void SetupTooltips()
        {
            _toolTip = new ToolTip();
            _toolTip.InitialDelay = 300;
            _toolTip.ShowAlways = true;
            _toolTip.BackColor = Theme.BgPrimary;
            _toolTip.ForeColor = Theme.TextPrimary;

            _toolTip.SetToolTip(btnMain, "Toggle hardware ID protection (Ctrl+P)");
            _toolTip.SetToolTip(btnGenerate, "Generate new random hardware IDs (Ctrl+G)");
            _toolTip.SetToolTip(btnFullSpoof, "Apply registry + AppInit system-wide spoof (Ctrl+F)");
            _toolTip.SetToolTip(btnInject, "Inject protection into a specific process (Ctrl+I)");
            _toolTip.SetToolTip(btnConfig, "Edit spoofed hardware ID values (Ctrl+E)");
            _toolTip.SetToolTip(btnAppInit, "Toggle AppInit DLL injection for all apps");
            _toolTip.SetToolTip(btnRestore, "Restore original hardware IDs (Ctrl+R)");
            _toolTip.SetToolTip(chkAutoStart, "Start HWID Changer when Windows starts");
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Control)
            {
                switch (e.KeyCode)
                {
                    case Keys.P: btnMain.PerformClick(); e.Handled = true; break;
                    case Keys.G: btnGenerate.PerformClick(); e.Handled = true; break;
                    case Keys.F: btnFullSpoof.PerformClick(); e.Handled = true; break;
                    case Keys.I: btnInject.PerformClick(); e.Handled = true; break;
                    case Keys.E: btnConfig.PerformClick(); e.Handled = true; break;
                    case Keys.R: btnRestore.PerformClick(); e.Handled = true; break;
                }
            }
            else if (e.KeyCode == Keys.F1)
            {
                ShowHelp();
                e.Handled = true;
            }
            else if (e.KeyCode == Keys.Escape)
            {
                this.WindowState = FormWindowState.Minimized;
                e.Handled = true;
            }
        }

        private void ShowHelp()
        {
            var help = "⌨️  Keyboard Shortcuts\n\n" +
                "Ctrl+P  -  Toggle protection\n" +
                "Ctrl+G  -  Generate new IDs\n" +
                "Ctrl+F  -  Full system spoof\n" +
                "Ctrl+I  -  Inject to process\n" +
                "Ctrl+E  -  Edit config\n" +
                "Ctrl+R  -  Restore original\n" +
                "Esc     -  Minimize to tray\n" +
                "F1      -  Show this help\n\n" +
                "💡 Tips:\n" +
                "• Right-click tray icon for quick actions\n" +
                "• Double-click tray to restore window";
            
            MessageBox.Show(help, "HWID Changer Help", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void ShowNotification(string title, string message, ToolTipIcon icon)
        {
            if (_trayIcon != null)
            {
                _trayIcon.BalloonTipTitle = title;
                _trayIcon.BalloonTipText = message;
                _trayIcon.BalloonTipIcon = icon;
                _trayIcon.ShowBalloonTip(3000);
            }
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
        
        private void BtnHelp_Click(object s, EventArgs e) { ShowHelp(); }

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

            navQuick.ForeColor = tab == 0 ? Theme.Accent : Theme.TextSecondary;
            navAdvanced.ForeColor = tab == 1 ? Theme.Accent : Theme.TextSecondary;
            navConfig.ForeColor = tab == 2 ? Theme.Accent : Theme.TextSecondary;

            navQuick.Font = new Font("Segoe UI", 9F, tab == 0 ? FontStyle.Bold : FontStyle.Regular);
            navAdvanced.Font = new Font("Segoe UI", 9F, tab == 1 ? FontStyle.Bold : FontStyle.Regular);
            navConfig.Font = new Font("Segoe UI", 9F, tab == 2 ? FontStyle.Bold : FontStyle.Regular);

            // Move indicator
            switch (tab)
            {
                case 0: navIndicator.Location = new Point(navQuick.Left, navIndicator.Top); navIndicator.Width = navQuick.Width - 10; break;
                case 1: navIndicator.Location = new Point(navAdvanced.Left, navIndicator.Top); navIndicator.Width = navAdvanced.Width - 10; break;
                case 2: navIndicator.Location = new Point(navConfig.Left, navIndicator.Top); navIndicator.Width = navConfig.Width - 10; break;
            }
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
                SetStatus("Protected", Theme.Success);
                Log("Protection enabled!", LT.OK);
            }
            else
            {
                Log("Failed: " + r.Error, LT.Error);
                SetStatus("Failed", Theme.Danger);
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
                _appInitEnabled = false;
                btnAppInit.Text = "Enable AppInit";
                UpdateMainButton();
                Log("Original values restored", LT.OK);
                txtDisk.Text = txtVolume.Text = txtGuid.Text = txtProduct.Text = txtMac.Text = txtBios.Text = "";
                ShowNotification("Restored", "Original hardware IDs restored", ToolTipIcon.Info);
            }
            else 
            {
                Log("Restore failed: " + r.Error, LT.Error);
                ShowNotification("Failed", "Restore failed", ToolTipIcon.Error);
            }
            SetBusy(false);
        }

        private void UpdateMainButton()
        {
            if (_isSpoofActive)
            {
                btnMain.Text = "Disable Protection";
                btnMain.ButtonColor = Theme.Danger;
                UpdateStatusCard("Protected", "Hardware IDs are spoofed", Theme.Success, "✓");
            }
            else
            {
                btnMain.Text = "Enable Protection";
                btnMain.ButtonColor = Theme.Accent;
                UpdateStatusCard("Not Protected", "Click below to enable protection", Theme.TextMuted, "○");
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
            var pid = ShowProcessDialog();
            if (pid <= 0) return;

            SetBusy(true);
            Log("Injecting to process " + pid + "...", LT.Info);
            var r = await _service.InjectToProcess(pid);
            if (r.Success) 
            {
                Log("Injection successful!", LT.OK);
                UpdateStatusCard("Injected", "Protection active (PID: " + pid + ")", Theme.Success, "✓");
                ShowNotification("Success", "Process injected successfully", ToolTipIcon.Info);
            }
            else 
            {
                Log("Injection failed: " + r.Error, LT.Error);
                ShowNotification("Failed", "Injection failed", ToolTipIcon.Error);
            }
            SetBusy(false);
        }

        private int ShowProcessDialog()
        {
            // Collect all processes
            var processes = Process.GetProcesses();
            var items = new System.Collections.Generic.List<ProcessInfo>();
            foreach (var p in processes)
            {
                try
                {
                    // Include all user processes
                    if (p.Id > 4 && p.SessionId > 0)
                    {
                        items.Add(new ProcessInfo { 
                            Name = p.ProcessName, 
                            Id = p.Id, 
                            Title = p.MainWindowTitle 
                        });
                    }
                }
                catch { }
            }
            items.Sort((a, b) => a.Name.CompareTo(b.Name));

            // Create dialog
            using (var dlg = new ProcessSelectDialog(items))
            {
                if (dlg.ShowDialog(this) == DialogResult.OK)
                    return dlg.SelectedPid;
            }
            return -1;
        }

        private class ProcessInfo
        {
            public string Name { get; set; }
            public int Id { get; set; }
            public string Title { get; set; }
        }

        private class ProcessSelectDialog : Form
        {
            private ListView _list;
            private TextBox _search;
            private Button _btnOk, _btnCancel, _btnRefresh;
            private System.Collections.Generic.List<ProcessInfo> _allItems;
            public int SelectedPid { get; private set; }

            public ProcessSelectDialog(System.Collections.Generic.List<ProcessInfo> items)
            {
                _allItems = items;
                
                Text = "Select Process";
                Size = new Size(420, 500);
                StartPosition = FormStartPosition.CenterParent;
                FormBorderStyle = FormBorderStyle.FixedDialog;
                MaximizeBox = false;
                MinimizeBox = false;
                BackColor = Theme.BgPrimary;

                // Title
                var lblTitle = new Label { 
                    Text = "💉  Inject to Process", 
                    Font = new Font("Segoe UI", 14F), 
                    ForeColor = Theme.TextPrimary,
                    Location = new Point(20, 16), 
                    AutoSize = true 
                };

                // Search box
                _search = new TextBox { 
                    Location = new Point(20, 52), 
                    Size = new Size(360, 28),
                    Font = new Font("Segoe UI", 10F),
                    Text = "",
                    ForeColor = Color.Gray
                };
                _search.GotFocus += (s, e) => { 
                    if (_search.ForeColor == Color.Gray) { _search.Text = ""; _search.ForeColor = Color.Black; }
                };
                _search.LostFocus += (s, e) => { 
                    if (string.IsNullOrEmpty(_search.Text)) { _search.Text = "Search process..."; _search.ForeColor = Color.Gray; }
                };
                _search.Text = "Search process...";
                _search.TextChanged += (s, e) => { if (_search.ForeColor != Color.Gray) FilterList(); };

                // Process list
                _list = new ListView {
                    Location = new Point(20, 88),
                    Size = new Size(360, 310),
                    View = View.Details,
                    FullRowSelect = true,
                    Font = new Font("Segoe UI", 9F),
                    BorderStyle = BorderStyle.FixedSingle
                };
                _list.Columns.Add("Process", 180);
                _list.Columns.Add("PID", 60);
                _list.Columns.Add("Window", 100);
                _list.DoubleClick += (s, e) => { if (_list.SelectedItems.Count > 0) { DialogResult = DialogResult.OK; Close(); } };
                
                PopulateList(items);

                // Buttons
                _btnRefresh = new Button { 
                    Text = "↻ Refresh", 
                    Location = new Point(20, 410), 
                    Size = new Size(90, 36),
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.BgSecondary,
                    ForeColor = Theme.TextPrimary
                };
                _btnRefresh.Click += (s, e) => RefreshProcesses();

                _btnOk = new Button { 
                    Text = "Inject", 
                    Location = new Point(210, 410), 
                    Size = new Size(80, 36),
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.Accent,
                    ForeColor = Color.White,
                    DialogResult = DialogResult.OK
                };

                _btnCancel = new Button { 
                    Text = "Cancel", 
                    Location = new Point(300, 410), 
                    Size = new Size(80, 36),
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.BgSecondary,
                    ForeColor = Theme.TextPrimary,
                    DialogResult = DialogResult.Cancel 
                };

                Controls.AddRange(new Control[] { lblTitle, _search, _list, _btnRefresh, _btnOk, _btnCancel });
                AcceptButton = _btnOk;
                CancelButton = _btnCancel;

                FormClosing += (s, e) => {
                    if (DialogResult == DialogResult.OK && _list.SelectedItems.Count > 0)
                    {
                        SelectedPid = int.Parse(_list.SelectedItems[0].SubItems[1].Text);
                    }
                };
            }

            private void PopulateList(System.Collections.Generic.IEnumerable<ProcessInfo> items)
            {
                _list.Items.Clear();
                foreach (var p in items)
                {
                    var item = new ListViewItem(p.Name);
                    item.SubItems.Add(p.Id.ToString());
                    item.SubItems.Add(string.IsNullOrEmpty(p.Title) ? "-" : p.Title);
                    _list.Items.Add(item);
                }
            }

            private void FilterList()
            {
                var filter = _search.Text.ToLower();
                if (string.IsNullOrEmpty(filter))
                {
                    PopulateList(_allItems);
                }
                else
                {
                    var filtered = new System.Collections.Generic.List<ProcessInfo>();
                    foreach (var p in _allItems)
                    {
                        if (p.Name.ToLower().Contains(filter) || 
                            (p.Title != null && p.Title.ToLower().Contains(filter)))
                            filtered.Add(p);
                    }
                    PopulateList(filtered);
                }
            }

            private void RefreshProcesses()
            {
                _allItems.Clear();
                var processes = Process.GetProcesses();
                foreach (var p in processes)
                {
                    try
                    {
                        if (p.Id > 4 && p.SessionId > 0)
                        {
                            _allItems.Add(new ProcessInfo { 
                                Name = p.ProcessName, 
                                Id = p.Id, 
                                Title = p.MainWindowTitle 
                            });
                        }
                    }
                    catch { }
                }
                _allItems.Sort((a, b) => a.Name.CompareTo(b.Name));
                FilterList();
            }
        }

        private bool _appInitEnabled = false;

        private async void BtnAppInit_Click(object s, EventArgs e)
        {
            // Show dialog with options
            var result = MessageBox.Show(
                _appInitEnabled 
                    ? "AppInit is currently ENABLED.\n\nClick Yes to DISABLE it."
                    : "AppInit will inject protection into ALL new applications system-wide.\n\nClick Yes to ENABLE it.",
                "AppInit DLL Injection",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question);

            if (result != DialogResult.Yes) return;

            SetBusy(true);
            
            if (_appInitEnabled)
            {
                Log("Disabling AppInit...", LT.Info);
                var r = await _service.DisableAppInit();
                if (r.Success) 
                { 
                    Log("AppInit disabled", LT.OK);
                    _appInitEnabled = false;
                    btnAppInit.Text = "Enable AppInit";
                }
                else Log("Failed: " + r.Error, LT.Error);
            }
            else
            {
                Log("Enabling AppInit (system-wide)...", LT.Info);
                var r = await _service.EnableAppInit();
                if (r.Success) 
                { 
                    Log("AppInit enabled - affects all new apps", LT.OK);
                    _appInitEnabled = true;
                    btnAppInit.Text = "Disable AppInit";
                    UpdateStatusCard("System Protected", "All new apps will have spoofed IDs", Theme.Success, "✓");
                }
                else Log("Failed: " + r.Error, LT.Error);
            }
            
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
            var result = MessageBox.Show(
                "Full System Spoof will:\n\n" +
                "• Modify registry hardware IDs\n" +
                "• Enable AppInit for all new apps\n\n" +
                "This affects the entire system. Continue?",
                "Full System Spoof",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning);

            if (result != DialogResult.Yes) return;

            SetBusy(true);
            Log("Applying full system spoof...", LT.Info);
            var r = await _service.FullUsermodeSpoof();
            if (r.Success) 
            { 
                Log("Full system spoof applied!", LT.OK); 
                _isSpoofActive = true; 
                _appInitEnabled = true;
                btnAppInit.Text = "Disable AppInit";
                UpdateMainButton();
                UpdateStatusCard("Fully Protected", "Registry + AppInit active", Theme.Success, "✓");
                ShowNotification("Protected", "Full system spoof applied", ToolTipIcon.Info);
            }
            else 
            {
                Log("Failed: " + r.Error, LT.Error);
                ShowNotification("Failed", "Full spoof failed", ToolTipIcon.Error);
            }
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
            if (busy) lblStatusText.Text = "Working...";
        }

        private void SetStatus(string text, Color color)
        {
            UpdateStatusCard(text, "", color, color == Theme.Success ? "✓" : (color == Theme.Danger ? "!" : "○"));
        }

        private void UpdateStatusCard(string title, string subtitle, Color iconColor, string icon)
        {
            if (lblStatus != null) lblStatus.Text = title;
            if (lblStatusSub != null) lblStatusSub.Text = subtitle;
            if (lblStatusIcon != null) 
            {
                // Use emoji for better visual
                if (icon == "✓")
                    lblStatusIcon.Text = "🛡️";
                else if (icon == "!")
                    lblStatusIcon.Text = "⚠️";
                else
                    lblStatusIcon.Text = "○";
                lblStatusIcon.ForeColor = iconColor;
            }
        }

        private enum LT { Info, OK, Warn, Error }

        private void Log(string msg, LT t)
        {
            Color c;
            switch (t)
            {
                case LT.OK: c = Theme.Success; break;
                case LT.Warn: c = Color.FromArgb(255, 140, 0); break;
                case LT.Error: c = Theme.Danger; break;
                default: c = Theme.TextSecondary; break;
            }
            
            if (lblStatusText != null)
            {
                lblStatusText.Text = msg;
                lblStatusText.ForeColor = c;
            }
        }

        private void OnOutput(string s) { if (InvokeRequired) BeginInvoke(new Action(() => Log(s, LT.Info))); else Log(s, LT.Info); }
        private void OnError(string s) { if (InvokeRequired) BeginInvoke(new Action(() => Log(s, LT.Error))); else Log(s, LT.Error); }

        // New button handlers
        private void BtnConfig_Click(object sender, EventArgs e)
        {
            var cfgPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "hwid_current.cfg");
            string content = "";
            if (File.Exists(cfgPath)) content = File.ReadAllText(cfgPath);

            using (var dlg = new Form())
            {
                dlg.Text = "Edit Configuration";
                dlg.Size = new Size(520, 500);
                dlg.StartPosition = FormStartPosition.CenterParent;
                dlg.FormBorderStyle = FormBorderStyle.FixedDialog;
                dlg.MaximizeBox = false;
                dlg.MinimizeBox = false;
                dlg.BackColor = Theme.BgPrimary;

                // Title with icon
                var lblTitle = new Label { 
                    Text = "⚙️  Hardware ID Configuration", 
                    Location = new Point(20, 16), 
                    AutoSize = true, 
                    Font = new Font("Segoe UI", 14F), 
                    ForeColor = Theme.TextPrimary 
                };

                var lblDesc = new Label { 
                    Text = "Edit the spoofed hardware identifiers below", 
                    Location = new Point(20, 44), 
                    AutoSize = true, 
                    Font = new Font("Segoe UI", 9F), 
                    ForeColor = Theme.TextSecondary 
                };

                var txt = new TextBox { 
                    Location = new Point(20, 76), 
                    Size = new Size(460, 320), 
                    Multiline = true, 
                    ScrollBars = ScrollBars.Vertical, 
                    Font = new Font("Consolas", 10F), 
                    Text = content,
                    BackColor = Theme.BgSecondary,
                    BorderStyle = BorderStyle.FixedSingle
                };

                var btnGenerate = new Button { 
                    Text = "🔄 Generate New", 
                    Location = new Point(20, 410), 
                    Size = new Size(120, 36),
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.BgSecondary,
                    ForeColor = Theme.TextPrimary
                };
                
                var btnSave = new Button { 
                    Text = "💾 Save", 
                    Location = new Point(280, 410), 
                    Size = new Size(95, 36), 
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.Success,
                    ForeColor = Color.White,
                    DialogResult = DialogResult.OK 
                };

                var btnCancel = new Button { 
                    Text = "Cancel", 
                    Location = new Point(385, 410), 
                    Size = new Size(95, 36), 
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.BgSecondary,
                    ForeColor = Theme.TextPrimary,
                    DialogResult = DialogResult.Cancel 
                };
                
                btnGenerate.Click += async (s2, e2) => {
                    btnGenerate.Enabled = false;
                    btnGenerate.Text = "Generating...";
                    var r = await _service.GenerateNewConfig();
                    if (r.Success && File.Exists(cfgPath))
                    {
                        txt.Text = File.ReadAllText(cfgPath);
                    }
                    btnGenerate.Text = "🔄 Generate New";
                    btnGenerate.Enabled = true;
                };

                dlg.Controls.AddRange(new Control[] { lblTitle, lblDesc, txt, btnSave, btnCancel, btnGenerate });
                dlg.AcceptButton = btnSave;
                dlg.CancelButton = btnCancel;

                if (dlg.ShowDialog(this) == DialogResult.OK)
                {
                    File.WriteAllText(cfgPath, txt.Text);
                    Log("Config saved", LT.OK);
                }
            }
        }

        private async void BtnRestore_Click(object sender, EventArgs e)
        {
            var result = MessageBox.Show(
                "This will restore your original hardware IDs.\nAre you sure?",
                "Restore Original", 
                MessageBoxButtons.YesNo, 
                MessageBoxIcon.Question);
                
            if (result == DialogResult.Yes)
            {
                await DoRestore();
            }
        }
    }
}
