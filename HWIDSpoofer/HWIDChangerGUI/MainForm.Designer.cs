namespace HWIDChangerGUI
{
    partial class MainForm
    {
        private System.ComponentModel.IContainer components = null;
        protected override void Dispose(bool disposing) { if (disposing && components != null) components.Dispose(); base.Dispose(disposing); }

        private void InitializeComponent()
        {
            this.SuspendLayout();

            // ===== FORM =====
            this.BackColor = Theme.BgSecondary;
            this.ClientSize = new System.Drawing.Size(380, 540);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Padding = new System.Windows.Forms.Padding(0);

            // ===== HEADER =====
            panelHeader = new System.Windows.Forms.Panel();
            panelHeader.BackColor = Theme.BgPrimary;
            panelHeader.Dock = System.Windows.Forms.DockStyle.Top;
            panelHeader.Height = 60;
            panelHeader.Cursor = System.Windows.Forms.Cursors.SizeAll;
            panelHeader.MouseDown += TitleMouseDown;
            panelHeader.MouseMove += TitleMouseMove;
            panelHeader.MouseUp += TitleMouseUp;

            lblTitle = new System.Windows.Forms.Label();
            lblTitle.Text = Lang.Get("AppTitle");
            lblTitle.Font = new System.Drawing.Font("Segoe UI", 12F);
            lblTitle.ForeColor = Theme.TextPrimary;
            lblTitle.AutoSize = true;
            lblTitle.Location = new System.Drawing.Point(24, 20);
            lblTitle.Cursor = System.Windows.Forms.Cursors.SizeAll;
            lblTitle.MouseDown += TitleMouseDown;
            lblTitle.MouseMove += TitleMouseMove;
            lblTitle.MouseUp += TitleMouseUp;

            btnClose = new System.Windows.Forms.Label();
            btnClose.Text = "×";
            btnClose.Font = new System.Drawing.Font("Segoe UI Light", 18F);
            btnClose.ForeColor = Theme.TextMuted;
            btnClose.Size = new System.Drawing.Size(40, 40);
            btnClose.Location = new System.Drawing.Point(330, 10);
            btnClose.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            btnClose.Cursor = System.Windows.Forms.Cursors.Hand;
            btnClose.Click += BtnClose_Click;
            btnClose.MouseEnter += (s, e) => { btnClose.ForeColor = Theme.Danger; };
            btnClose.MouseLeave += (s, e) => { btnClose.ForeColor = Theme.TextMuted; };

            btnMin = new System.Windows.Forms.Label();
            btnMin.Text = "−";
            btnMin.Font = new System.Drawing.Font("Segoe UI", 16F);
            btnMin.ForeColor = Theme.TextMuted;
            btnMin.Size = new System.Drawing.Size(40, 40);
            btnMin.Location = new System.Drawing.Point(290, 10);
            btnMin.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            btnMin.Cursor = System.Windows.Forms.Cursors.Hand;
            btnMin.Click += BtnMin_Click;
            btnMin.MouseEnter += (s, e) => { btnMin.ForeColor = Theme.TextSecondary; };
            btnMin.MouseLeave += (s, e) => { btnMin.ForeColor = Theme.TextMuted; };

            btnHelp = new System.Windows.Forms.Label();
            btnHelp.Text = "?";
            btnHelp.Font = new System.Drawing.Font("Segoe UI", 12F);
            btnHelp.ForeColor = Theme.TextMuted;
            btnHelp.Size = new System.Drawing.Size(32, 32);
            btnHelp.Location = new System.Drawing.Point(210, 14);
            btnHelp.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            btnHelp.Cursor = System.Windows.Forms.Cursors.Hand;
            btnHelp.Click += BtnHelp_Click;
            btnHelp.MouseEnter += (s, e) => { btnHelp.ForeColor = Theme.Accent; };
            btnHelp.MouseLeave += (s, e) => { btnHelp.ForeColor = Theme.TextMuted; };

            btnLang = new System.Windows.Forms.Label();
            btnLang.Text = Lang.IsChinese ? "EN" : "中";
            btnLang.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Bold);
            btnLang.ForeColor = Theme.TextMuted;
            btnLang.Size = new System.Drawing.Size(32, 32);
            btnLang.Location = new System.Drawing.Point(250, 14);
            btnLang.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            btnLang.Cursor = System.Windows.Forms.Cursors.Hand;
            btnLang.Click += BtnLang_Click;
            btnLang.MouseEnter += (s, e) => { btnLang.ForeColor = Theme.Accent; };
            btnLang.MouseLeave += (s, e) => { btnLang.ForeColor = Theme.TextMuted; };

            panelHeader.Controls.Add(lblTitle);
            panelHeader.Controls.Add(btnHelp);
            panelHeader.Controls.Add(btnLang);
            panelHeader.Controls.Add(btnClose);
            panelHeader.Controls.Add(btnMin);

            // ===== STATUS CARD =====
            cardStatus = new ModernCard();
            cardStatus.Location = new System.Drawing.Point(24, 76);
            cardStatus.Size = new System.Drawing.Size(332, 88);

            lblStatusIcon = new System.Windows.Forms.Label();
            lblStatusIcon.Text = "○";
            lblStatusIcon.Font = new System.Drawing.Font("Segoe UI Light", 28F);
            lblStatusIcon.ForeColor = Theme.TextMuted;
            lblStatusIcon.Size = new System.Drawing.Size(52, 52);
            lblStatusIcon.Location = new System.Drawing.Point(18, 18);
            lblStatusIcon.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;

            lblStatus = new System.Windows.Forms.Label();
            lblStatus.Text = Lang.Get("StatusNotProtected");
            lblStatus.Font = new System.Drawing.Font("Segoe UI", 13F);
            lblStatus.ForeColor = Theme.TextPrimary;
            lblStatus.AutoSize = true;
            lblStatus.Location = new System.Drawing.Point(72, 22);

            lblStatusSub = new System.Windows.Forms.Label();
            lblStatusSub.Text = Lang.Get("SubNotProtected");
            lblStatusSub.Font = new System.Drawing.Font("Segoe UI", 9F);
            lblStatusSub.ForeColor = Theme.TextSecondary;
            lblStatusSub.AutoSize = true;
            lblStatusSub.Location = new System.Drawing.Point(72, 48);

            cardStatus.Controls.Add(lblStatusIcon);
            cardStatus.Controls.Add(lblStatus);
            cardStatus.Controls.Add(lblStatusSub);

            // ===== MAIN BUTTON =====
            btnMain = new ModernButton();
            btnMain.Text = Lang.Get("BtnEnable");
            btnMain.ButtonColor = Theme.Accent;
            btnMain.Font = new System.Drawing.Font("Segoe UI", 11F);
            btnMain.Size = new System.Drawing.Size(332, 52);
            btnMain.Location = new System.Drawing.Point(24, 180);
            btnMain.Click += BtnMain_Click;

            // ===== ACTION BUTTONS =====
            btnGenerate = new ModernButton();
            btnGenerate.Text = Lang.Get("BtnGenerate");
            btnGenerate.ButtonColor = Theme.Primary;
            btnGenerate.Outline = true;
            btnGenerate.Font = new System.Drawing.Font("Segoe UI", 9.5F);
            btnGenerate.Size = new System.Drawing.Size(158, 44);
            btnGenerate.Location = new System.Drawing.Point(24, 248);
            btnGenerate.Click += BtnGenerate_Click;

            btnFullSpoof = new ModernButton();
            btnFullSpoof.Text = Lang.Get("BtnFullSpoof");
            btnFullSpoof.ButtonColor = Theme.Primary;
            btnFullSpoof.Outline = true;
            btnFullSpoof.Font = new System.Drawing.Font("Segoe UI", 9.5F);
            btnFullSpoof.Size = new System.Drawing.Size(158, 44);
            btnFullSpoof.Location = new System.Drawing.Point(198, 248);
            btnFullSpoof.Click += BtnFullSpoof_Click;

            // ===== ADVANCED SECTION =====
            lblAdvanced = new System.Windows.Forms.Label();
            lblAdvanced.Text = Lang.Get("LblMoreOptions");
            lblAdvanced.Font = new System.Drawing.Font("Segoe UI", 9F);
            lblAdvanced.ForeColor = Theme.TextMuted;
            lblAdvanced.AutoSize = true;
            lblAdvanced.Location = new System.Drawing.Point(24, 316);

            btnInject = new ModernButton();
            btnInject.Text = Lang.Get("BtnInject");
            btnInject.ButtonColor = Theme.Primary;
            btnInject.Outline = true;
            btnInject.Font = new System.Drawing.Font("Segoe UI", 9.5F);
            btnInject.Size = new System.Drawing.Size(158, 44);
            btnInject.Location = new System.Drawing.Point(24, 344);
            btnInject.Click += BtnInject_Click;

            btnConfig = new ModernButton();
            btnConfig.Text = Lang.Get("BtnConfig");
            btnConfig.ButtonColor = Theme.Primary;
            btnConfig.Outline = true;
            btnConfig.Font = new System.Drawing.Font("Segoe UI", 9.5F);
            btnConfig.Size = new System.Drawing.Size(158, 44);
            btnConfig.Location = new System.Drawing.Point(198, 344);
            btnConfig.Click += BtnConfig_Click;

            btnAppInit = new ModernButton();
            btnAppInit.Text = Lang.Get("BtnAppInitEnable");
            btnAppInit.ButtonColor = Theme.Primary;
            btnAppInit.Outline = true;
            btnAppInit.Font = new System.Drawing.Font("Segoe UI", 9.5F);
            btnAppInit.Size = new System.Drawing.Size(158, 44);
            btnAppInit.Location = new System.Drawing.Point(24, 400);
            btnAppInit.Click += BtnAppInit_Click;

            btnRestore = new ModernButton();
            btnRestore.Text = Lang.Get("BtnRestore");
            btnRestore.ButtonColor = Theme.Primary;
            btnRestore.Outline = true;
            btnRestore.Font = new System.Drawing.Font("Segoe UI", 9.5F);
            btnRestore.Size = new System.Drawing.Size(158, 44);
            btnRestore.Location = new System.Drawing.Point(198, 400);
            btnRestore.Click += BtnRestore_Click;

            // ===== FOOTER =====
            panelFooter = new System.Windows.Forms.Panel();
            panelFooter.BackColor = Theme.BgSecondary;
            panelFooter.Dock = System.Windows.Forms.DockStyle.Bottom;
            panelFooter.Height = 52;

            lblStatusText = new System.Windows.Forms.Label();
            lblStatusText.Text = Lang.Get("LblReady");
            lblStatusText.Font = new System.Drawing.Font("Segoe UI", 9F);
            lblStatusText.ForeColor = Theme.TextSecondary;
            lblStatusText.AutoSize = true;
            lblStatusText.Location = new System.Drawing.Point(24, 18);

            chkAutoStart = new System.Windows.Forms.CheckBox();
            chkAutoStart.Text = Lang.Get("LblAutoStart");
            chkAutoStart.Font = new System.Drawing.Font("Segoe UI", 9F);
            chkAutoStart.ForeColor = Theme.TextMuted;
            chkAutoStart.AutoSize = true;
            chkAutoStart.Location = new System.Drawing.Point(270, 16);
            chkAutoStart.CheckedChanged += ChkAutoStart_Changed;

            progressBar = new System.Windows.Forms.ProgressBar();
            progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            progressBar.Size = new System.Drawing.Size(332, 3);
            progressBar.Location = new System.Drawing.Point(24, 0);
            progressBar.Visible = false;

            panelFooter.Controls.Add(lblStatusText);
            panelFooter.Controls.Add(chkAutoStart);
            panelFooter.Controls.Add(progressBar);

            // Hidden controls for compatibility
            panelQuick = new System.Windows.Forms.Panel() { Visible = false };
            panelAdvanced = new System.Windows.Forms.Panel() { Visible = false };
            panelConfig = new System.Windows.Forms.Panel() { Visible = false };
            panelNav = new System.Windows.Forms.Panel() { Visible = false };
            panelStatusBar = new System.Windows.Forms.Panel() { Visible = false };
            navQuick = new System.Windows.Forms.Label() { Visible = false };
            navAdvanced = new System.Windows.Forms.Label() { Visible = false };
            navConfig = new System.Windows.Forms.Label() { Visible = false };
            navIndicator = new System.Windows.Forms.Panel() { Visible = false };
            cardLog = new ModernCard() { Visible = false };
            txtLog = new System.Windows.Forms.RichTextBox() { Visible = false };
            cboProcess = new System.Windows.Forms.ComboBox() { Visible = false };
            btnRefresh = new ModernButton() { Visible = false };
            btnAppInitOff = new ModernButton() { Visible = false };
            lblInjectTitle = new System.Windows.Forms.Label() { Visible = false };
            lblSystemTitle = new System.Windows.Forms.Label() { Visible = false };
            lblDisk = new System.Windows.Forms.Label() { Visible = false };
            lblVolume = new System.Windows.Forms.Label() { Visible = false };
            lblGuid = new System.Windows.Forms.Label() { Visible = false };
            lblProduct = new System.Windows.Forms.Label() { Visible = false };
            lblMac = new System.Windows.Forms.Label() { Visible = false };
            lblBios = new System.Windows.Forms.Label() { Visible = false };
            txtDisk = new ModernTextBox() { Visible = false };
            txtVolume = new ModernTextBox() { Visible = false };
            txtGuid = new ModernTextBox() { Visible = false };
            txtProduct = new ModernTextBox() { Visible = false };
            txtMac = new ModernTextBox() { Visible = false };
            txtBios = new ModernTextBox() { Visible = false };
            btnSave = new ModernButton() { Visible = false };
            btnImport = new ModernButton() { Visible = false };
            btnExport = new ModernButton() { Visible = false };

            // ===== ADD CONTROLS =====
            this.Controls.Add(cardStatus);
            this.Controls.Add(btnMain);
            this.Controls.Add(btnGenerate);
            this.Controls.Add(btnFullSpoof);
            this.Controls.Add(lblAdvanced);
            this.Controls.Add(btnInject);
            this.Controls.Add(btnConfig);
            this.Controls.Add(btnAppInit);
            this.Controls.Add(btnRestore);
            this.Controls.Add(panelFooter);
            this.Controls.Add(panelHeader);

            this.ResumeLayout(false);
        }

        // Main controls
        private System.Windows.Forms.Panel panelHeader, panelFooter;
        private System.Windows.Forms.Label lblTitle, btnClose, btnMin, btnHelp, btnLang;
        private ModernCard cardStatus;
        private System.Windows.Forms.Label lblStatusIcon, lblStatus, lblStatusSub, lblStatusText;
        private ModernButton btnMain, btnGenerate, btnFullSpoof;
        private System.Windows.Forms.Label lblAdvanced;
        private ModernButton btnInject, btnConfig, btnAppInit, btnRestore;
        private System.Windows.Forms.CheckBox chkAutoStart;
        private System.Windows.Forms.ProgressBar progressBar;

        // Hidden compatibility controls
        private System.Windows.Forms.Panel panelQuick, panelAdvanced, panelConfig, panelNav, panelStatusBar;
        private System.Windows.Forms.Panel navIndicator;
        private System.Windows.Forms.Label navQuick, navAdvanced, navConfig;
        private ModernCard cardLog;
        private System.Windows.Forms.RichTextBox txtLog;
        private System.Windows.Forms.ComboBox cboProcess;
        private ModernButton btnRefresh, btnAppInitOff;
        private System.Windows.Forms.Label lblInjectTitle, lblSystemTitle;
        private System.Windows.Forms.Label lblDisk, lblVolume, lblGuid, lblProduct, lblMac, lblBios;
        private ModernTextBox txtDisk, txtVolume, txtGuid, txtProduct, txtMac, txtBios;
        private ModernButton btnSave, btnImport, btnExport;
    }
}
