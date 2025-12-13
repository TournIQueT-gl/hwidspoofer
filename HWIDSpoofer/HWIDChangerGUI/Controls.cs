using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace HWIDChangerGUI
{
    /// <summary>
    /// Soft & Friendly Design System
    /// </summary>
    public static class Theme
    {
        // Soft palette
        public static readonly Color Primary = Color.FromArgb(64, 64, 64);
        public static readonly Color Accent = Color.FromArgb(88, 136, 220);      // Softer blue
        public static readonly Color Success = Color.FromArgb(72, 172, 104);     // Soft green
        public static readonly Color Danger = Color.FromArgb(220, 100, 100);     // Soft red
        public static readonly Color Warning = Color.FromArgb(240, 180, 80);     // Soft orange
        
        public static readonly Color BgPrimary = Color.White;
        public static readonly Color BgSecondary = Color.FromArgb(252, 252, 254);  // Very light blue-gray
        public static readonly Color BgCard = Color.White;
        public static readonly Color Border = Color.FromArgb(235, 238, 242);       // Soft border
        public static readonly Color Shadow = Color.FromArgb(20, 0, 0, 0);         // Very subtle shadow
        
        public static readonly Color TextPrimary = Color.FromArgb(48, 48, 56);
        public static readonly Color TextSecondary = Color.FromArgb(108, 112, 120);
        public static readonly Color TextMuted = Color.FromArgb(168, 172, 180);
        
        public const int Radius = 12;          // More rounded
        public const int RadiusSmall = 8;
        public const int ButtonHeight = 44;
        public const int InputHeight = 40;
        public const int Spacing = 20;
    }

    // Compatibility aliases
    public static class Fluent
    {
        public static Color AccentPrimary { get { return Theme.Accent; } }
        public static Color Success { get { return Theme.Success; } }
        public static Color Danger { get { return Theme.Danger; } }
        public static Color Warning { get { return Color.FromArgb(255, 140, 0); } }
        public static Color TextPrimary { get { return Theme.TextPrimary; } }
        public static Color TextSecondary { get { return Theme.TextSecondary; } }
        public static Color TextDisabled { get { return Theme.TextMuted; } }
        public static Color Surface { get { return Theme.BgPrimary; } }
        public static Color Background { get { return Theme.BgSecondary; } }
        public static Color Border { get { return Theme.Border; } }
        public static Color TextOnAccent { get { return Color.White; } }
        public const int Radius = 6;
        public const int ControlHeight = 36;
    }

    /// <summary>
    /// Soft Button with gentle hover
    /// </summary>
    public class ModernButton : Control
    {
        private Color _bgColor = Theme.Accent;
        private Color _textColor = Color.White;
        private bool _hover = false;
        private bool _press = false;
        private bool _outline = false;

        public Color ButtonColor
        {
            get { return _bgColor; }
            set { _bgColor = value; Invalidate(); }
        }

        public Color TextColor
        {
            get { return _textColor; }
            set { _textColor = value; Invalidate(); }
        }

        public bool Outline
        {
            get { return _outline; }
            set { _outline = value; Invalidate(); }
        }

        public ModernButton()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | 
                     ControlStyles.DoubleBuffer | ControlStyles.ResizeRedraw, true);
            Font = new Font("Segoe UI", 9.5F);
            Cursor = Cursors.Hand;
            Size = new Size(120, Theme.ButtonHeight);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;

            var rect = new Rectangle(1, 1, Width - 3, Height - 3);
            Color bg = _bgColor;
            Color fg = _textColor;

            if (_outline)
            {
                // Soft outline style - use light background instead of transparent
                bg = Theme.BgSecondary;
                fg = Theme.TextPrimary;
                
                if (_press) bg = Color.FromArgb(235, 238, 242);
                else if (_hover) bg = Color.FromArgb(242, 245, 248);

                using (var path = RoundedRect(rect, Theme.RadiusSmall))
                {
                    using (var brush = new SolidBrush(bg)) g.FillPath(brush, path);
                }
            }
            else
            {
                // Filled style with soft shadow
                if (_press) bg = Darken(_bgColor, 0.1f);
                else if (_hover) bg = Lighten(_bgColor, 0.06f);

                // Subtle shadow
                var shadowRect = new Rectangle(rect.X + 1, rect.Y + 2, rect.Width, rect.Height);
                using (var shadowPath = RoundedRect(shadowRect, Theme.RadiusSmall))
                using (var shadowBrush = new SolidBrush(Color.FromArgb(25, 0, 0, 0)))
                {
                    g.FillPath(shadowBrush, shadowPath);
                }

                using (var path = RoundedRect(rect, Theme.RadiusSmall))
                using (var brush = new SolidBrush(bg))
                {
                    g.FillPath(brush, path);
                }
            }

            var textRect = new Rectangle(0, 0, Width, Height);
            TextRenderer.DrawText(g, Text, Font, textRect, fg,
                TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
        }

        protected override void OnMouseEnter(EventArgs e) 
        { 
            _hover = true; 
            Cursor = Cursors.Hand;
            Invalidate(); 
            base.OnMouseEnter(e); 
        }
        
        protected override void OnMouseLeave(EventArgs e) 
        { 
            _hover = false; 
            _press = false; 
            Invalidate(); 
            base.OnMouseLeave(e); 
        }
        
        protected override void OnMouseDown(MouseEventArgs e) 
        { 
            _press = true; 
            // Scale down effect
            Invalidate(); 
            base.OnMouseDown(e); 
        }
        
        protected override void OnMouseUp(MouseEventArgs e) 
        { 
            _press = false; 
            Invalidate(); 
            base.OnMouseUp(e); 
        }

        private GraphicsPath RoundedRect(Rectangle r, int rad)
        {
            var path = new GraphicsPath();
            int d = rad * 2;
            path.AddArc(r.X, r.Y, d, d, 180, 90);
            path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }

        public void PerformClick()
        {
            OnClick(EventArgs.Empty);
        }

        private Color Darken(Color c, float amt) { return Color.FromArgb(c.A, (int)(c.R * (1 - amt)), (int)(c.G * (1 - amt)), (int)(c.B * (1 - amt))); }
        private Color Lighten(Color c, float amt) { return Color.FromArgb(c.A, Math.Min(255, (int)(c.R + (255 - c.R) * amt)), Math.Min(255, (int)(c.G + (255 - c.G) * amt)), Math.Min(255, (int)(c.B + (255 - c.B) * amt))); }
    }

    /// <summary>
    /// Clean TextBox
    /// </summary>
    public class ModernTextBox : UserControl
    {
        private TextBox _tb;
        private bool _focus = false;

        public override string Text
        {
            get { return _tb != null ? _tb.Text : ""; }
            set { if (_tb != null) _tb.Text = value; }
        }

        public bool ReadOnly
        {
            get { return _tb != null && _tb.ReadOnly; }
            set { if (_tb != null) _tb.ReadOnly = value; }
        }

        public ModernTextBox()
        {
            Size = new Size(200, Theme.InputHeight);
            BackColor = Color.Transparent;

            _tb = new TextBox
            {
                BorderStyle = BorderStyle.None,
                Font = new Font("Segoe UI", 9F),
                BackColor = Theme.BgPrimary,
                ForeColor = Theme.TextPrimary
            };

            _tb.GotFocus += (s, e) => { _focus = true; Invalidate(); };
            _tb.LostFocus += (s, e) => { _focus = false; Invalidate(); };
            _tb.TextChanged += (s, e) => OnTextChanged(e);

            Controls.Add(_tb);
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            if (_tb == null) return;
            _tb.Location = new Point(10, (Height - _tb.PreferredHeight) / 2);
            _tb.Width = Width - 20;
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;

            var rect = new Rectangle(0, 0, Width - 1, Height - 1);

            using (var path = RoundedRect(rect, 4))
            {
                using (var brush = new SolidBrush(Theme.BgPrimary)) g.FillPath(brush, path);
                using (var pen = new Pen(_focus ? Theme.Accent : Theme.Border, _focus ? 2f : 1f)) g.DrawPath(pen, path);
            }
        }

        private GraphicsPath RoundedRect(Rectangle r, int rad)
        {
            var path = new GraphicsPath();
            int d = rad * 2;
            path.AddArc(r.X, r.Y, d, d, 180, 90);
            path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }
    }

    /// <summary>
    /// Soft Card with subtle shadow
    /// </summary>
    public class ModernCard : Panel
    {
        public ModernCard()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint |
                     ControlStyles.DoubleBuffer | ControlStyles.ResizeRedraw, true);
            BackColor = Color.Transparent;
            Padding = new Padding(20);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;

            // Soft shadow layers
            for (int i = 3; i >= 1; i--)
            {
                var shadowRect = new Rectangle(i, i + 1, Width - i * 2 - 1, Height - i * 2 - 1);
                using (var path = RoundedRect(shadowRect, Theme.Radius))
                using (var brush = new SolidBrush(Color.FromArgb(8, 0, 0, 0)))
                {
                    g.FillPath(brush, path);
                }
            }

            // Main card
            var rect = new Rectangle(0, 0, Width - 1, Height - 1);
            using (var path = RoundedRect(rect, Theme.Radius))
            {
                using (var brush = new SolidBrush(Theme.BgCard)) g.FillPath(brush, path);
            }
        }

        private GraphicsPath RoundedRect(Rectangle r, int rad)
        {
            var path = new GraphicsPath();
            int d = rad * 2;
            path.AddArc(r.X, r.Y, d, d, 180, 90);
            path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
            path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }
    }
}
