using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace HWIDChangerGUI
{
    /// <summary>
    /// Modern Minimal Style - Clean and Simple
    /// </summary>
    public static class Modern
    {
        // Colors - Muted and Professional
        public static readonly Color Primary = ColorTranslator.FromHtml("#475569");      // Slate gray
        public static readonly Color Success = ColorTranslator.FromHtml("#059669");      // Muted green
        public static readonly Color Warning = ColorTranslator.FromHtml("#D97706");      // Muted amber
        public static readonly Color Danger = ColorTranslator.FromHtml("#DC2626");       // Muted red
        public static readonly Color Purple = ColorTranslator.FromHtml("#7C3AED");       // Muted purple
        public static readonly Color Cyan = ColorTranslator.FromHtml("#0891B2");         // Muted cyan

        public static readonly Color Background = ColorTranslator.FromHtml("#F8FAFC");   // Light gray bg
        public static readonly Color Surface = Color.White;
        public static readonly Color Border = ColorTranslator.FromHtml("#E2E8F0");       // Light border
        public static readonly Color BorderFocus = ColorTranslator.FromHtml("#475569");  // Slate focus

        public static readonly Color Text = ColorTranslator.FromHtml("#1E293B");         // Dark text
        public static readonly Color TextLight = ColorTranslator.FromHtml("#64748B");    // Gray text
        public static readonly Color TextWhite = Color.White;

        // Unified sizes
        public const int ControlHeight = 40;
        public const int Radius = 8;
        public const int Padding = 12;
    }

    /// <summary>
    /// Modern Button - Clean flat style
    /// </summary>
    public class ModernButton : Control
    {
        private Color _buttonColor = Modern.Primary;
        private Color _textColor = Modern.TextWhite;
        private bool _isHovered = false;
        private bool _isPressed = false;

        public Color ButtonColor
        {
            get { return _buttonColor; }
            set { _buttonColor = value; Invalidate(); }
        }

        public Color TextColor
        {
            get { return _textColor; }
            set { _textColor = value; Invalidate(); }
        }

        public ModernButton()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | 
                     ControlStyles.DoubleBuffer | ControlStyles.ResizeRedraw, true);
            Font = new Font("Segoe UI", 10F, FontStyle.Bold);
            Cursor = Cursors.Hand;
            Size = new Size(120, Modern.ControlHeight);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;

            Rectangle rect = new Rectangle(0, 0, Width - 1, Height - 1);

            // Calculate color based on state
            Color bgColor = _buttonColor;
            if (_isPressed)
            {
                bgColor = DarkenColor(_buttonColor, 0.15f);
            }
            else if (_isHovered)
            {
                bgColor = DarkenColor(_buttonColor, 0.08f);
            }

            // Draw rounded background
            using (GraphicsPath path = CreateRoundedPath(rect, Modern.Radius))
            using (SolidBrush brush = new SolidBrush(bgColor))
            {
                g.FillPath(brush, path);
            }

            // Draw text centered
            TextRenderer.DrawText(g, Text, Font, rect, _textColor,
                TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
        }

        protected override void OnMouseEnter(EventArgs e)
        {
            _isHovered = true;
            Invalidate();
            base.OnMouseEnter(e);
        }

        protected override void OnMouseLeave(EventArgs e)
        {
            _isHovered = false;
            _isPressed = false;
            Invalidate();
            base.OnMouseLeave(e);
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            _isPressed = true;
            Invalidate();
            base.OnMouseDown(e);
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            _isPressed = false;
            Invalidate();
            base.OnMouseUp(e);
        }

        private GraphicsPath CreateRoundedPath(Rectangle rect, int radius)
        {
            GraphicsPath path = new GraphicsPath();
            int d = radius * 2;
            path.AddArc(rect.X, rect.Y, d, d, 180, 90);
            path.AddArc(rect.Right - d, rect.Y, d, d, 270, 90);
            path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90);
            path.AddArc(rect.X, rect.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }

        private Color DarkenColor(Color color, float amount)
        {
            return Color.FromArgb(color.A,
                (int)Math.Max(0, color.R * (1 - amount)),
                (int)Math.Max(0, color.G * (1 - amount)),
                (int)Math.Max(0, color.B * (1 - amount)));
        }
    }

    /// <summary>
    /// Modern TextBox - Clean bordered input
    /// </summary>
    public class ModernTextBox : Control
    {
        private TextBox _inner;
        private bool _isFocused = false;

        public override string Text
        {
            get { return _inner != null ? _inner.Text : string.Empty; }
            set { if (_inner != null) _inner.Text = value; }
        }

        public bool ReadOnly
        {
            get { return _inner != null ? _inner.ReadOnly : false; }
            set { if (_inner != null) _inner.ReadOnly = value; }
        }

        public ModernTextBox()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint |
                     ControlStyles.DoubleBuffer | ControlStyles.ResizeRedraw, true);

            Size = new Size(200, Modern.ControlHeight);
            
            _inner = new TextBox();
            _inner.BorderStyle = BorderStyle.None;
            _inner.Font = new Font("Segoe UI", 10F);
            _inner.BackColor = Modern.Surface;
            _inner.ForeColor = Modern.Text;
            
            _inner.GotFocus += (s, e) => { _isFocused = true; Invalidate(); };
            _inner.LostFocus += (s, e) => { _isFocused = false; Invalidate(); };
            _inner.TextChanged += (s, e) => { OnTextChanged(e); };

            Controls.Add(_inner);
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            if (_inner == null) return;
            int pad = Modern.Padding;
            _inner.Location = new Point(pad, (Height - _inner.PreferredHeight) / 2);
            _inner.Width = Width - pad * 2;
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;

            Rectangle rect = new Rectangle(0, 0, Width - 1, Height - 1);

            // Background
            using (GraphicsPath path = CreateRoundedPath(rect, Modern.Radius))
            using (SolidBrush brush = new SolidBrush(Modern.Surface))
            {
                g.FillPath(brush, path);
            }

            // Border
            Color borderColor = _isFocused ? Modern.BorderFocus : Modern.Border;
            using (GraphicsPath path = CreateRoundedPath(rect, Modern.Radius))
            using (Pen pen = new Pen(borderColor, _isFocused ? 2f : 1f))
            {
                g.DrawPath(pen, path);
            }
        }

        private GraphicsPath CreateRoundedPath(Rectangle rect, int radius)
        {
            GraphicsPath path = new GraphicsPath();
            int d = radius * 2;
            path.AddArc(rect.X, rect.Y, d, d, 180, 90);
            path.AddArc(rect.Right - d, rect.Y, d, d, 270, 90);
            path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90);
            path.AddArc(rect.X, rect.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }
    }

    /// <summary>
    /// Modern ComboBox - Styled dropdown
    /// </summary>
    public class ModernComboBox : ComboBox
    {
        public ModernComboBox()
        {
            FlatStyle = FlatStyle.Flat;
            Font = new Font("Segoe UI", 10F);
            BackColor = Modern.Surface;
            ForeColor = Modern.Text;
            DropDownStyle = ComboBoxStyle.DropDownList;
            ItemHeight = Modern.ControlHeight - 16;
        }

        protected override void OnCreateControl()
        {
            base.OnCreateControl();
            // Set height through ItemHeight
            this.DropDownHeight = 200;
        }
    }

    /// <summary>
    /// Modern Card - Container with shadow
    /// </summary>
    public class ModernCard : Panel
    {
        public ModernCard()
        {
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint |
                     ControlStyles.DoubleBuffer | ControlStyles.ResizeRedraw, true);
            BackColor = Color.Transparent;
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;

            Rectangle rect = new Rectangle(2, 2, Width - 5, Height - 5);

            // Soft shadow
            Rectangle shadowRect = new Rectangle(rect.X + 2, rect.Y + 2, rect.Width, rect.Height);
            using (GraphicsPath shadowPath = CreateRoundedPath(shadowRect, Modern.Radius))
            using (SolidBrush shadowBrush = new SolidBrush(Color.FromArgb(15, 0, 0, 0)))
            {
                g.FillPath(shadowBrush, shadowPath);
            }

            // Card background
            using (GraphicsPath path = CreateRoundedPath(rect, Modern.Radius))
            using (SolidBrush brush = new SolidBrush(Modern.Surface))
            {
                g.FillPath(brush, path);
            }

            // Border
            using (GraphicsPath path = CreateRoundedPath(rect, Modern.Radius))
            using (Pen pen = new Pen(Modern.Border, 1f))
            {
                g.DrawPath(pen, path);
            }
        }

        private GraphicsPath CreateRoundedPath(Rectangle rect, int radius)
        {
            GraphicsPath path = new GraphicsPath();
            int d = radius * 2;
            path.AddArc(rect.X, rect.Y, d, d, 180, 90);
            path.AddArc(rect.Right - d, rect.Y, d, d, 270, 90);
            path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90);
            path.AddArc(rect.X, rect.Bottom - d, d, d, 90, 90);
            path.CloseFigure();
            return path;
        }
    }
}
