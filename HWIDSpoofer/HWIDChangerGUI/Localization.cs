using System;
using System.Globalization;
using System.Collections.Generic;
using System.IO;

namespace HWIDChangerGUI
{
    /// <summary>
    /// Simple localization system with Chinese and English support
    /// </summary>
    public static class Lang
    {
        private static string _currentLang = "en";
        private static Dictionary<string, Dictionary<string, string>> _strings;
        private static readonly string _langFile = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "lang.cfg");

        static Lang()
        {
            // Try to load saved preference first
            if (File.Exists(_langFile))
            {
                try
                {
                    _currentLang = File.ReadAllText(_langFile).Trim();
                    if (_currentLang != "zh" && _currentLang != "en")
                        _currentLang = "en";
                }
                catch { }
            }
            else
            {
                // Detect system language
                var culture = CultureInfo.CurrentUICulture;
                _currentLang = culture.TwoLetterISOLanguageName == "zh" ? "zh" : "en";
            }
            
            InitStrings();
        }

        public static string CurrentLanguage { get { return _currentLang; } }
        
        public static bool IsChinese { get { return _currentLang == "zh"; } }

        public static void SetLanguage(string lang)
        {
            _currentLang = (lang == "zh") ? "zh" : "en";
        }

        public static void ToggleLanguage()
        {
            _currentLang = (_currentLang == "zh") ? "en" : "zh";
            SavePreference();
        }

        private static void SavePreference()
        {
            try
            {
                File.WriteAllText(_langFile, _currentLang);
            }
            catch { }
        }

        public static string Get(string key)
        {
            if (_strings.ContainsKey(key))
            {
                var dict = _strings[key];
                if (dict.ContainsKey(_currentLang))
                    return dict[_currentLang];
                if (dict.ContainsKey("en"))
                    return dict["en"];
            }
            return key; // Fallback to key itself
        }

        private static void InitStrings()
        {
            _strings = new Dictionary<string, Dictionary<string, string>>();

            // ===== Window =====
            Add("AppTitle", "HWID Changer", "HWID 修改器");
            
            // ===== Status =====
            Add("StatusProtected", "Protected", "已保护");
            Add("StatusNotProtected", "Not Protected", "未保护");
            Add("StatusFullyProtected", "Fully Protected", "完全保护");
            Add("StatusInjected", "Injected", "已注入");
            Add("StatusCoreNotFound", "Core Not Found", "核心未找到");
            
            Add("SubProtected", "Your hardware IDs are spoofed", "您的硬件ID已被伪装");
            Add("SubNotProtected", "Click below to enable protection", "点击下方按钮启用保护");
            Add("SubFullyProtected", "Registry + AppInit active", "注册表 + AppInit 已激活");
            Add("SubCoreNotFound", "HWIDChanger.exe is missing", "缺少 HWIDChanger.exe");
            
            // ===== Buttons =====
            Add("BtnEnable", "Enable Protection", "启用保护");
            Add("BtnDisable", "Disable Protection", "禁用保护");
            Add("BtnGenerate", "Generate New IDs", "生成新ID");
            Add("BtnFullSpoof", "Full System Spoof", "全系统伪装");
            Add("BtnInject", "Inject to Process", "注入进程");
            Add("BtnConfig", "Edit Config", "编辑配置");
            Add("BtnAppInitEnable", "Enable AppInit", "启用 AppInit");
            Add("BtnAppInitDisable", "Disable AppInit", "禁用 AppInit");
            Add("BtnRestore", "Restore Original", "恢复原始");
            Add("BtnRefresh", "Refresh", "刷新");
            Add("BtnSave", "Save", "保存");
            Add("BtnCancel", "Cancel", "取消");
            Add("BtnGenerateNew", "Generate New", "生成新值");
            
            // ===== Labels =====
            Add("LblMoreOptions", "More options", "更多选项");
            Add("LblAutoStart", "Auto-start", "开机启动");
            Add("LblReady", "Ready", "就绪");
            
            // ===== Dialogs =====
            Add("DlgProcessTitle", "Select Process", "选择进程");
            Add("DlgProcessDesc", "Select a process to inject:", "选择要注入的进程：");
            Add("DlgProcessSearch", "Search process...", "搜索进程...");
            Add("DlgConfigTitle", "Edit Configuration", "编辑配置");
            Add("DlgConfigDesc", "Edit the spoofed hardware identifiers below", "编辑下方的伪装硬件标识符");
            
            Add("DlgFullSpoofTitle", "Full System Spoof", "全系统伪装");
            Add("DlgFullSpoofMsg", "Full System Spoof will:\n\n• Modify registry hardware IDs\n• Enable AppInit for all new apps\n\nThis affects the entire system. Continue?",
                "全系统伪装将会：\n\n• 修改注册表硬件ID\n• 为所有新程序启用 AppInit\n\n这将影响整个系统。确定继续？");
            
            Add("DlgRestoreTitle", "Restore Original", "恢复原始");
            Add("DlgRestoreMsg", "This will restore your original hardware IDs.\nAre you sure?",
                "这将恢复您的原始硬件ID。\n确定要恢复吗？");
            
            Add("DlgAppInitEnableMsg", "AppInit will inject protection into ALL new applications system-wide.\n\nClick Yes to ENABLE it.",
                "AppInit 将为系统中所有新启动的程序注入保护。\n\n点击【是】启用。");
            Add("DlgAppInitDisableMsg", "AppInit is currently ENABLED.\n\nClick Yes to DISABLE it.",
                "AppInit 当前已启用。\n\n点击【是】禁用。");
            Add("DlgAppInitTitle", "AppInit DLL Injection", "AppInit DLL 注入");
            
            // ===== Log Messages =====
            Add("LogReady", "Ready", "就绪");
            Add("LogWelcome", "Welcome! Press F1 for help", "欢迎！按 F1 查看帮助");
            Add("LogProtectionActive", "Protection active", "保护已激活");
            Add("LogGenerating", "Generating new IDs...", "正在生成新ID...");
            Add("LogGenerated", "New IDs generated!", "新ID已生成！");
            Add("LogGenerateFailed", "Generation failed", "生成失败");
            Add("LogApplying", "Applying protection...", "正在应用保护...");
            Add("LogApplied", "Protection applied!", "保护已应用！");
            Add("LogApplyFailed", "Apply failed", "应用失败");
            Add("LogRestoring", "Restoring original hardware IDs...", "正在恢复原始硬件ID...");
            Add("LogRestored", "Original values restored", "原始值已恢复");
            Add("LogRestoreFailed", "Restore failed", "恢复失败");
            Add("LogInjecting", "Injecting to process {0}...", "正在注入进程 {0}...");
            Add("LogInjected", "Injection successful!", "注入成功！");
            Add("LogInjectFailed", "Injection failed", "注入失败");
            Add("LogAppInitEnabled", "AppInit enabled - affects all new apps", "AppInit 已启用 - 影响所有新程序");
            Add("LogAppInitDisabled", "AppInit disabled", "AppInit 已禁用");
            Add("LogFullSpoof", "Applying full system spoof...", "正在应用全系统伪装...");
            Add("LogFullSpoofDone", "Full system spoof applied!", "全系统伪装已应用！");
            Add("LogConfigSaved", "Config saved", "配置已保存");
            Add("LogCoreNotFound", "HWIDChanger.exe not found", "未找到 HWIDChanger.exe");
            Add("LogNoProcess", "No suitable processes found", "未找到合适的进程");
            
            // ===== Notifications =====
            Add("NotifySuccess", "Success", "成功");
            Add("NotifyFailed", "Failed", "失败");
            Add("NotifyProtected", "Protected", "已保护");
            Add("NotifyRestored", "Restored", "已恢复");
            Add("NotifyInjected", "Process injected successfully", "进程注入成功");
            Add("NotifyFullSpoof", "Full system spoof applied", "全系统伪装已应用");
            Add("NotifyOriginalRestored", "Original hardware IDs restored", "原始硬件ID已恢复");
            Add("NotifyError", "Error", "错误");
            Add("NotifyCoreNotFound", "Core component not found", "核心组件未找到");
            
            // ===== Help =====
            Add("HelpTitle", "HWID Changer Help", "HWID 修改器帮助");
            Add("HelpContent", 
                "⌨️  Keyboard Shortcuts\n\n" +
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
                "• Double-click tray to restore window",
                
                "⌨️  快捷键\n\n" +
                "Ctrl+P  -  切换保护状态\n" +
                "Ctrl+G  -  生成新ID\n" +
                "Ctrl+F  -  全系统伪装\n" +
                "Ctrl+I  -  注入进程\n" +
                "Ctrl+E  -  编辑配置\n" +
                "Ctrl+R  -  恢复原始\n" +
                "Esc     -  最小化到托盘\n" +
                "F1      -  显示此帮助\n\n" +
                "💡 提示：\n" +
                "• 右键点击托盘图标可快速操作\n" +
                "• 双击托盘图标恢复窗口");
            
            // ===== Tooltips =====
            Add("TipMain", "Toggle hardware ID protection (Ctrl+P)", "切换硬件ID保护 (Ctrl+P)");
            Add("TipGenerate", "Generate new random hardware IDs (Ctrl+G)", "生成新的随机硬件ID (Ctrl+G)");
            Add("TipFullSpoof", "Apply registry + AppInit system-wide spoof (Ctrl+F)", "应用注册表 + AppInit 全系统伪装 (Ctrl+F)");
            Add("TipInject", "Inject protection into a specific process (Ctrl+I)", "向指定进程注入保护 (Ctrl+I)");
            Add("TipConfig", "Edit spoofed hardware ID values (Ctrl+E)", "编辑伪装的硬件ID值 (Ctrl+E)");
            Add("TipAppInit", "Toggle AppInit DLL injection for all apps", "切换所有程序的 AppInit DLL 注入");
            Add("TipRestore", "Restore original hardware IDs (Ctrl+R)", "恢复原始硬件ID (Ctrl+R)");
            Add("TipAutoStart", "Start HWID Changer when Windows starts", "Windows 启动时自动运行");
            
            // ===== Tray Menu =====
            Add("TrayOpen", "Open", "打开");
            Add("TrayQuickSpoof", "Quick Spoof", "快速伪装");
            Add("TrayRestore", "Restore", "恢复");
            Add("TrayQuit", "Quit", "退出");
            
            // ===== Process Dialog =====
            Add("ColProcess", "Process", "进程");
            Add("ColPID", "PID", "PID");
            Add("ColWindow", "Window", "窗口");
        }

        private static void Add(string key, string en, string zh)
        {
            _strings[key] = new Dictionary<string, string> 
            { 
                { "en", en }, 
                { "zh", zh } 
            };
        }
    }
}

