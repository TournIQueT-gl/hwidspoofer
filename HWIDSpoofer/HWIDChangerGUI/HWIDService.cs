using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace HWIDChangerGUI
{
    /// <summary>
    /// HWID Service - Wrapper for HWIDChanger.exe calls
    /// </summary>
    public class HWIDService
    {
        private readonly string _exePath;
        private readonly string _workingDir;

        public event Action<string> OutputReceived;
        public event Action<string> ErrorReceived;

        public HWIDService()
        {
            _workingDir = AppDomain.CurrentDomain.BaseDirectory;
            _exePath = Path.Combine(_workingDir, "HWIDChanger.exe");
        }

        public bool IsExeAvailable 
        { 
            get { return File.Exists(_exePath); }
        }

        /// <summary>
        /// Generate new HWID config
        /// </summary>
        public async Task<ExecuteResult> GenerateNewConfig()
        {
            return await ExecuteAsync("--generate");
        }

        /// <summary>
        /// Save current config
        /// </summary>
        public async Task<ExecuteResult> SaveConfig(string configFile = "hwid_current.cfg")
        {
            return await ExecuteAsync(string.Format("--save \"{0}\"", configFile));
        }

        /// <summary>
        /// Load config
        /// </summary>
        public async Task<ExecuteResult> LoadConfig(string configFile)
        {
            return await ExecuteAsync(string.Format("--load \"{0}\"", configFile));
        }

        /// <summary>
        /// Apply registry spoof (system-wide)
        /// </summary>
        public async Task<ExecuteResult> ApplyRegistrySpoof()
        {
            return await ExecuteAsync("--registry-global");
        }

        /// <summary>
        /// Restore registry
        /// </summary>
        public async Task<ExecuteResult> RestoreRegistry()
        {
            return await ExecuteAsync("--restore-registry");
        }

        /// <summary>
        /// Enable AppInit injection (system-wide usermode)
        /// </summary>
        public async Task<ExecuteResult> EnableAppInit()
        {
            return await ExecuteAsync("--enable-appinit");
        }

        /// <summary>
        /// Disable AppInit injection
        /// </summary>
        public async Task<ExecuteResult> DisableAppInit()
        {
            return await ExecuteAsync("--disable-appinit");
        }

        /// <summary>
        /// Full usermode spoof (registry + AppInit)
        /// </summary>
        public async Task<ExecuteResult> FullUsermodeSpoof()
        {
            return await ExecuteAsync("--full-usermode");
        }

        /// <summary>
        /// Inject to process by PID
        /// </summary>
        public async Task<ExecuteResult> InjectToProcess(int pid)
        {
            return await ExecuteAsync(string.Format("--inject {0}", pid));
        }

        /// <summary>
        /// Inject to process by name
        /// </summary>
        public async Task<ExecuteResult> InjectToProcess(string processName)
        {
            return await ExecuteAsync(string.Format("--inject-name \"{0}\"", processName));
        }

        /// <summary>
        /// Show current HWID info
        /// </summary>
        public async Task<ExecuteResult> ShowCurrentHWID()
        {
            return await ExecuteAsync("--show");
        }

        /// <summary>
        /// Execute HWIDChanger command
        /// </summary>
        private async Task<ExecuteResult> ExecuteAsync(string arguments)
        {
            if (!IsExeAvailable)
            {
                return new ExecuteResult
                {
                    Success = false,
                    Output = "",
                    Error = "HWIDChanger.exe not found\nPath: " + _exePath
                };
            }

            var result = new ExecuteResult();
            var outputBuilder = new StringBuilder();
            var errorBuilder = new StringBuilder();

            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = _exePath,
                    Arguments = arguments,
                    WorkingDirectory = _workingDir,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8
                };

                using (var process = new Process())
                {
                    process.StartInfo = startInfo;
                    
                    process.OutputDataReceived += (s, e) =>
                    {
                        if (e.Data != null)
                        {
                            outputBuilder.AppendLine(e.Data);
                            if (OutputReceived != null)
                                OutputReceived(e.Data);
                        }
                    };

                    process.ErrorDataReceived += (s, e) =>
                    {
                        if (e.Data != null)
                        {
                            errorBuilder.AppendLine(e.Data);
                            if (ErrorReceived != null)
                                ErrorReceived(e.Data);
                        }
                    };

                    process.Start();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    await Task.Run(() => process.WaitForExit(30000)); // 30s timeout

                    if (!process.HasExited)
                    {
                        process.Kill();
                        result.Success = false;
                        result.Error = "Command timeout";
                    }
                    else
                    {
                        result.Success = process.ExitCode == 0;
                        result.ExitCode = process.ExitCode;
                    }
                }

                result.Output = outputBuilder.ToString();
                result.Error = errorBuilder.ToString();
            }
            catch (Exception ex)
            {
                result.Success = false;
                result.Error = "Execution error: " + ex.Message;
            }

            return result;
        }

        /// <summary>
        /// Execute as administrator
        /// </summary>
        public ExecuteResult ExecuteAsAdmin(string arguments)
        {
            if (!IsExeAvailable)
            {
                return new ExecuteResult
                {
                    Success = false,
                    Error = "HWIDChanger.exe not found\nPath: " + _exePath
                };
            }

            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = _exePath,
                    Arguments = arguments,
                    WorkingDirectory = _workingDir,
                    UseShellExecute = true,
                    Verb = "runas"
                };

                var process = Process.Start(startInfo);
                if (process != null)
                    process.WaitForExit(30000);

                return new ExecuteResult
                {
                    Success = process != null && process.ExitCode == 0,
                    ExitCode = process != null ? process.ExitCode : -1
                };
            }
            catch (Exception ex)
            {
                return new ExecuteResult
                {
                    Success = false,
                    Error = "Execution error: " + ex.Message
                };
            }
        }

        /// <summary>
        /// Read config file
        /// </summary>
        public string ReadConfigFile(string fileName = "hwid_current.cfg")
        {
            var path = Path.Combine(_workingDir, fileName);
            if (File.Exists(path))
            {
                return File.ReadAllText(path, Encoding.UTF8);
            }
            return null;
        }

        /// <summary>
        /// Check if backup exists
        /// </summary>
        public bool HasBackup()
        {
            string backupPath = Path.Combine(_workingDir, "hwid_backup.dat");
            if (!File.Exists(backupPath)) return false;
            
            // Check if file has actual content (not empty)
            var info = new FileInfo(backupPath);
            return info.Length > 0;
        }
    }

    public class ExecuteResult
    {
        public bool Success { get; set; }
        public string Output { get; set; }
        public string Error { get; set; }
        public int ExitCode { get; set; }

        public ExecuteResult()
        {
            Output = "";
            Error = "";
        }
    }
}
