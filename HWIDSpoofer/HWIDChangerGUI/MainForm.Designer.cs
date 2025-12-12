namespace HWIDChangerGUI
{
    partial class MainForm
    {
        private System.ComponentModel.IContainer components = null;
        protected override void Dispose(bool disposing) { if (disposing && components != null) components.Dispose(); base.Dispose(disposing); }

        private void InitializeComponent()
        {
            // Window
            this.panelTitle = new System.Windows.Forms.Panel();
            this.btnClose = new System.Windows.Forms.Label();
            this.btnMin = new System.Windows.Forms.Label();
            this.lblTitle = new System.Windows.Forms.Label();

            // Status
            this.lblStatus = new System.Windows.Forms.Label();
            this.lblStatusIcon = new System.Windows.Forms.Label();

            // Nav
            this.navQuick = new System.Windows.Forms.Label();
            this.navAdvanced = new System.Windows.Forms.Label();
            this.navConfig = new System.Windows.Forms.Label();

            // Quick
            this.panelQuick = new System.Windows.Forms.Panel();
            this.btnMain = new ModernButton();
            this.btnGenerate = new ModernButton();

            // Advanced
            this.panelAdvanced = new System.Windows.Forms.Panel();
            this.lblInjectTitle = new System.Windows.Forms.Label();
            this.cboProcess = new System.Windows.Forms.ComboBox();
            this.btnRefresh = new ModernButton();
            this.btnInject = new ModernButton();
            this.lblSystemTitle = new System.Windows.Forms.Label();
            this.btnAppInit = new ModernButton();
            this.btnAppInitOff = new ModernButton();
            this.btnFullSpoof = new ModernButton();

            // Config
            this.panelConfig = new System.Windows.Forms.Panel();
            this.lblDisk = new System.Windows.Forms.Label();
            this.txtDisk = new ModernTextBox();
            this.lblVolume = new System.Windows.Forms.Label();
            this.txtVolume = new ModernTextBox();
            this.lblGuid = new System.Windows.Forms.Label();
            this.txtGuid = new ModernTextBox();
            this.lblProduct = new System.Windows.Forms.Label();
            this.txtProduct = new ModernTextBox();
            this.lblMac = new System.Windows.Forms.Label();
            this.txtMac = new ModernTextBox();
            this.lblBios = new System.Windows.Forms.Label();
            this.txtBios = new ModernTextBox();
            this.btnSave = new ModernButton();
            this.btnImport = new ModernButton();
            this.btnExport = new ModernButton();

            // Log
            this.cardLog = new ModernCard();
            this.txtLog = new System.Windows.Forms.RichTextBox();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.chkAutoStart = new System.Windows.Forms.CheckBox();

            this.SuspendLayout();

            // ===== FORM =====
            this.BackColor = Modern.Background;
            this.ClientSize = new System.Drawing.Size(520, 700);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Font = new System.Drawing.Font("Segoe UI", 10F);
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;

            // ===== TITLE BAR =====
            this.panelTitle.BackColor = Modern.Surface;
            this.panelTitle.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelTitle.Height = 52;
            this.panelTitle.Cursor = System.Windows.Forms.Cursors.SizeAll;
            this.panelTitle.MouseDown += new System.Windows.Forms.MouseEventHandler(TitleMouseDown);
            this.panelTitle.MouseMove += new System.Windows.Forms.MouseEventHandler(TitleMouseMove);
            this.panelTitle.MouseUp += new System.Windows.Forms.MouseEventHandler(TitleMouseUp);

            // Close button
            this.btnClose.BackColor = Modern.Danger;
            this.btnClose.ForeColor = System.Drawing.Color.White;
            this.btnClose.Text = "✕";
            this.btnClose.Font = new System.Drawing.Font("Segoe UI", 8F, System.Drawing.FontStyle.Bold);
            this.btnClose.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.btnClose.Size = new System.Drawing.Size(20, 20);
            this.btnClose.Location = new System.Drawing.Point(20, 16);
            this.btnClose.Cursor = System.Windows.Forms.Cursors.Hand;
            this.btnClose.Click += new System.EventHandler(BtnClose_Click);

            // Minimize button
            this.btnMin.BackColor = Modern.Warning;
            this.btnMin.ForeColor = System.Drawing.Color.White;
            this.btnMin.Text = "−";
            this.btnMin.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Bold);
            this.btnMin.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.btnMin.Size = new System.Drawing.Size(20, 20);
            this.btnMin.Location = new System.Drawing.Point(48, 16);
            this.btnMin.Cursor = System.Windows.Forms.Cursors.Hand;
            this.btnMin.Click += new System.EventHandler(BtnMin_Click);

            // Title
            this.lblTitle.Text = "HWID Changer";
            this.lblTitle.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold);
            this.lblTitle.ForeColor = Modern.Text;
            this.lblTitle.AutoSize = true;
            this.lblTitle.Location = new System.Drawing.Point(190, 16);
            this.lblTitle.Cursor = System.Windows.Forms.Cursors.SizeAll;
            this.lblTitle.MouseDown += new System.Windows.Forms.MouseEventHandler(TitleMouseDown);
            this.lblTitle.MouseMove += new System.Windows.Forms.MouseEventHandler(TitleMouseMove);
            this.lblTitle.MouseUp += new System.Windows.Forms.MouseEventHandler(TitleMouseUp);

            this.panelTitle.Controls.Add(this.btnClose);
            this.panelTitle.Controls.Add(this.btnMin);
            this.panelTitle.Controls.Add(this.lblTitle);

            // ===== STATUS =====
            this.lblStatusIcon.Text = "●";
            this.lblStatusIcon.Font = new System.Drawing.Font("Segoe UI", 14F);
            this.lblStatusIcon.ForeColor = Modern.TextLight;
            this.lblStatusIcon.Location = new System.Drawing.Point(32, 72);
            this.lblStatusIcon.AutoSize = true;

            this.lblStatus.Text = "Not Protected";
            this.lblStatus.Font = new System.Drawing.Font("Segoe UI", 20F, System.Drawing.FontStyle.Bold);
            this.lblStatus.ForeColor = Modern.Text;
            this.lblStatus.Location = new System.Drawing.Point(56, 66);
            this.lblStatus.AutoSize = true;

            // ===== NAV =====
            this.navQuick.Text = "Quick";
            this.navQuick.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.navQuick.ForeColor = Modern.Primary;
            this.navQuick.Location = new System.Drawing.Point(32, 112);
            this.navQuick.AutoSize = true;
            this.navQuick.Cursor = System.Windows.Forms.Cursors.Hand;
            this.navQuick.Click += new System.EventHandler(TabQuick_Click);

            this.navAdvanced.Text = "Advanced";
            this.navAdvanced.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.navAdvanced.ForeColor = Modern.TextLight;
            this.navAdvanced.Location = new System.Drawing.Point(100, 112);
            this.navAdvanced.AutoSize = true;
            this.navAdvanced.Cursor = System.Windows.Forms.Cursors.Hand;
            this.navAdvanced.Click += new System.EventHandler(TabAdvanced_Click);

            this.navConfig.Text = "Config";
            this.navConfig.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.navConfig.ForeColor = Modern.TextLight;
            this.navConfig.Location = new System.Drawing.Point(200, 112);
            this.navConfig.AutoSize = true;
            this.navConfig.Cursor = System.Windows.Forms.Cursors.Hand;
            this.navConfig.Click += new System.EventHandler(TabConfig_Click);

            // ===== QUICK PANEL =====
            this.panelQuick.Location = new System.Drawing.Point(32, 150);
            this.panelQuick.Size = new System.Drawing.Size(456, 120);
            this.panelQuick.BackColor = System.Drawing.Color.Transparent;

            this.btnMain.Text = "Enable Protection";
            this.btnMain.ButtonColor = Modern.Success;
            this.btnMain.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.btnMain.Size = new System.Drawing.Size(456, Modern.ControlHeight);
            this.btnMain.Location = new System.Drawing.Point(0, 0);
            this.btnMain.Click += new System.EventHandler(BtnMain_Click);

            this.btnGenerate.Text = "Generate New IDs";
            this.btnGenerate.ButtonColor = Modern.Primary;
            this.btnGenerate.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Bold);
            this.btnGenerate.Size = new System.Drawing.Size(456, Modern.ControlHeight);
            this.btnGenerate.Location = new System.Drawing.Point(0, 52);
            this.btnGenerate.Click += new System.EventHandler(BtnGenerate_Click);

            this.panelQuick.Controls.Add(this.btnMain);
            this.panelQuick.Controls.Add(this.btnGenerate);

            // ===== ADVANCED PANEL =====
            this.panelAdvanced.Location = new System.Drawing.Point(32, 150);
            this.panelAdvanced.Size = new System.Drawing.Size(456, 280);
            this.panelAdvanced.BackColor = System.Drawing.Color.Transparent;
            this.panelAdvanced.Visible = false;

            this.lblInjectTitle.Text = "Inject to Process";
            this.lblInjectTitle.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.lblInjectTitle.ForeColor = Modern.TextLight;
            this.lblInjectTitle.Location = new System.Drawing.Point(0, 0);

            this.cboProcess.Location = new System.Drawing.Point(0, 24);
            this.cboProcess.Size = new System.Drawing.Size(300, Modern.ControlHeight);
            this.cboProcess.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cboProcess.Font = new System.Drawing.Font("Segoe UI", 10F);

            this.btnRefresh.Text = "↻";
            this.btnRefresh.ButtonColor = Modern.Primary;
            this.btnRefresh.Size = new System.Drawing.Size(Modern.ControlHeight, Modern.ControlHeight);
            this.btnRefresh.Location = new System.Drawing.Point(308, 24);
            this.btnRefresh.Click += new System.EventHandler(BtnRefresh_Click);

            this.btnInject.Text = "Inject";
            this.btnInject.ButtonColor = Modern.Primary;
            this.btnInject.Size = new System.Drawing.Size(96, Modern.ControlHeight);
            this.btnInject.Location = new System.Drawing.Point(360, 24);
            this.btnInject.Click += new System.EventHandler(BtnInject_Click);

            this.lblSystemTitle.Text = "System-wide Protection";
            this.lblSystemTitle.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            this.lblSystemTitle.ForeColor = Modern.TextLight;
            this.lblSystemTitle.Location = new System.Drawing.Point(0, 88);

            this.btnAppInit.Text = "Enable AppInit";
            this.btnAppInit.ButtonColor = Modern.Primary;
            this.btnAppInit.Size = new System.Drawing.Size(150, Modern.ControlHeight);
            this.btnAppInit.Location = new System.Drawing.Point(0, 112);
            this.btnAppInit.Click += new System.EventHandler(BtnAppInit_Click);

            this.btnAppInitOff.Text = "Disable AppInit";
            this.btnAppInitOff.ButtonColor = Modern.Primary;
            this.btnAppInitOff.Size = new System.Drawing.Size(150, Modern.ControlHeight);
            this.btnAppInitOff.Location = new System.Drawing.Point(160, 112);
            this.btnAppInitOff.Click += new System.EventHandler(BtnAppInitOff_Click);

            this.btnFullSpoof.Text = "Full System Spoof";
            this.btnFullSpoof.ButtonColor = Modern.Primary;
            this.btnFullSpoof.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.btnFullSpoof.Size = new System.Drawing.Size(456, Modern.ControlHeight);
            this.btnFullSpoof.Location = new System.Drawing.Point(0, 176);
            this.btnFullSpoof.Click += new System.EventHandler(BtnFullSpoof_Click);

            this.panelAdvanced.Controls.Add(this.lblInjectTitle);
            this.panelAdvanced.Controls.Add(this.cboProcess);
            this.panelAdvanced.Controls.Add(this.btnRefresh);
            this.panelAdvanced.Controls.Add(this.btnInject);
            this.panelAdvanced.Controls.Add(this.lblSystemTitle);
            this.panelAdvanced.Controls.Add(this.btnAppInit);
            this.panelAdvanced.Controls.Add(this.btnAppInitOff);
            this.panelAdvanced.Controls.Add(this.btnFullSpoof);

            // ===== CONFIG PANEL =====
            this.panelConfig.Location = new System.Drawing.Point(32, 150);
            this.panelConfig.Size = new System.Drawing.Size(456, 360);
            this.panelConfig.BackColor = System.Drawing.Color.Transparent;
            this.panelConfig.Visible = false;

            int cy = 0;
            int lw = 80;
            int inputW = 368;
            int rh = 50;

            this.lblDisk.Text = "Disk";
            this.lblDisk.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.lblDisk.ForeColor = Modern.TextLight;
            this.lblDisk.AutoSize = true;
            this.lblDisk.Location = new System.Drawing.Point(8, cy + 12);

            this.txtDisk.Location = new System.Drawing.Point(lw, cy);
            this.txtDisk.Size = new System.Drawing.Size(inputW, Modern.ControlHeight);

            cy += rh;
            this.lblVolume.Text = "Volume";
            this.lblVolume.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.lblVolume.ForeColor = Modern.TextLight;
            this.lblVolume.AutoSize = true;
            this.lblVolume.Location = new System.Drawing.Point(8, cy + 12);

            this.txtVolume.Location = new System.Drawing.Point(lw, cy);
            this.txtVolume.Size = new System.Drawing.Size(inputW, Modern.ControlHeight);

            cy += rh;
            this.lblGuid.Text = "GUID";
            this.lblGuid.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.lblGuid.ForeColor = Modern.TextLight;
            this.lblGuid.AutoSize = true;
            this.lblGuid.Location = new System.Drawing.Point(8, cy + 12);

            this.txtGuid.Location = new System.Drawing.Point(lw, cy);
            this.txtGuid.Size = new System.Drawing.Size(inputW, Modern.ControlHeight);

            cy += rh;
            this.lblProduct.Text = "Product";
            this.lblProduct.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.lblProduct.ForeColor = Modern.TextLight;
            this.lblProduct.AutoSize = true;
            this.lblProduct.Location = new System.Drawing.Point(8, cy + 12);

            this.txtProduct.Location = new System.Drawing.Point(lw, cy);
            this.txtProduct.Size = new System.Drawing.Size(inputW, Modern.ControlHeight);

            cy += rh;
            this.lblMac.Text = "MAC";
            this.lblMac.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.lblMac.ForeColor = Modern.TextLight;
            this.lblMac.AutoSize = true;
            this.lblMac.Location = new System.Drawing.Point(8, cy + 12);

            this.txtMac.Location = new System.Drawing.Point(lw, cy);
            this.txtMac.Size = new System.Drawing.Size(inputW, Modern.ControlHeight);

            cy += rh;
            this.lblBios.Text = "BIOS";
            this.lblBios.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.lblBios.ForeColor = Modern.TextLight;
            this.lblBios.AutoSize = true;
            this.lblBios.Location = new System.Drawing.Point(8, cy + 12);

            this.txtBios.Location = new System.Drawing.Point(lw, cy);
            this.txtBios.Size = new System.Drawing.Size(inputW, Modern.ControlHeight);

            cy += rh + 16;

            this.btnSave.Text = "Save";
            this.btnSave.ButtonColor = Modern.Primary;
            this.btnSave.Size = new System.Drawing.Size(110, Modern.ControlHeight);
            this.btnSave.Location = new System.Drawing.Point(lw, cy);
            this.btnSave.Click += new System.EventHandler(BtnSave_Click);

            this.btnImport.Text = "Import";
            this.btnImport.ButtonColor = Modern.Primary;
            this.btnImport.Size = new System.Drawing.Size(110, Modern.ControlHeight);
            this.btnImport.Location = new System.Drawing.Point(lw + 118, cy);
            this.btnImport.Click += new System.EventHandler(BtnImport_Click);

            this.btnExport.Text = "Export";
            this.btnExport.ButtonColor = Modern.Primary;
            this.btnExport.Size = new System.Drawing.Size(110, Modern.ControlHeight);
            this.btnExport.Location = new System.Drawing.Point(lw + 236, cy);
            this.btnExport.Click += new System.EventHandler(BtnExport_Click);

            this.panelConfig.Controls.Add(this.lblDisk);
            this.panelConfig.Controls.Add(this.txtDisk);
            this.panelConfig.Controls.Add(this.lblVolume);
            this.panelConfig.Controls.Add(this.txtVolume);
            this.panelConfig.Controls.Add(this.lblGuid);
            this.panelConfig.Controls.Add(this.txtGuid);
            this.panelConfig.Controls.Add(this.lblProduct);
            this.panelConfig.Controls.Add(this.txtProduct);
            this.panelConfig.Controls.Add(this.lblMac);
            this.panelConfig.Controls.Add(this.txtMac);
            this.panelConfig.Controls.Add(this.lblBios);
            this.panelConfig.Controls.Add(this.txtBios);
            this.panelConfig.Controls.Add(this.btnSave);
            this.panelConfig.Controls.Add(this.btnImport);
            this.panelConfig.Controls.Add(this.btnExport);

            // ===== LOG =====
            this.cardLog.Location = new System.Drawing.Point(32, 540);
            this.cardLog.Size = new System.Drawing.Size(456, 100);

            this.txtLog.BackColor = Modern.Surface;
            this.txtLog.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtLog.Font = new System.Drawing.Font("Consolas", 9F);
            this.txtLog.ForeColor = Modern.TextLight;
            this.txtLog.Location = new System.Drawing.Point(12, 12);
            this.txtLog.Size = new System.Drawing.Size(424, 68);
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.Vertical;

            this.cardLog.Controls.Add(this.txtLog);

            this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.progressBar.MarqueeAnimationSpeed = 25;
            this.progressBar.Location = new System.Drawing.Point(32, 532);
            this.progressBar.Size = new System.Drawing.Size(456, 3);
            this.progressBar.Visible = false;

            this.chkAutoStart.Text = "Launch at startup";
            this.chkAutoStart.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.chkAutoStart.ForeColor = Modern.TextLight;
            this.chkAutoStart.Location = new System.Drawing.Point(32, 652);
            this.chkAutoStart.AutoSize = true;
            this.chkAutoStart.CheckedChanged += new System.EventHandler(ChkAutoStart_Changed);

            // ===== ADD =====
            this.Controls.Add(this.lblStatusIcon);
            this.Controls.Add(this.lblStatus);
            this.Controls.Add(this.navQuick);
            this.Controls.Add(this.navAdvanced);
            this.Controls.Add(this.navConfig);
            this.Controls.Add(this.panelQuick);
            this.Controls.Add(this.panelAdvanced);
            this.Controls.Add(this.panelConfig);
            this.Controls.Add(this.cardLog);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.chkAutoStart);
            this.Controls.Add(this.panelTitle);

            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private System.Windows.Forms.Panel panelTitle;
        private System.Windows.Forms.Label btnClose, btnMin;
        private System.Windows.Forms.Label lblTitle, lblStatusIcon, lblStatus;
        private System.Windows.Forms.Label navQuick, navAdvanced, navConfig;
        private System.Windows.Forms.Panel panelQuick, panelAdvanced, panelConfig;
        private ModernButton btnMain, btnGenerate;
        private System.Windows.Forms.Label lblInjectTitle, lblSystemTitle;
        private System.Windows.Forms.ComboBox cboProcess;
        private ModernButton btnRefresh, btnInject, btnAppInit, btnAppInitOff, btnFullSpoof;
        private System.Windows.Forms.Label lblDisk, lblVolume, lblGuid, lblProduct, lblMac, lblBios;
        private ModernTextBox txtDisk, txtVolume, txtGuid, txtProduct, txtMac, txtBios;
        private ModernButton btnSave, btnImport, btnExport;
        private ModernCard cardLog;
        private System.Windows.Forms.RichTextBox txtLog;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.CheckBox chkAutoStart;
    }
}
