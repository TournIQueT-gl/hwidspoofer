#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <cstdarg>

namespace hwid {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERR = 4,      // ERROR is a macro in Windows
    NONE = 5
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }
    
    // 初始化日志
    bool initialize(const std::wstring& log_path = L"");
    void shutdown();
    
    // 设置日志级别
    void set_level(LogLevel level) { m_level = level; }
    LogLevel get_level() const { return m_level; }
    
    // 日志输出
    void log(LogLevel level, const char* format, ...);
    void log(LogLevel level, const wchar_t* format, ...);
    
    // 便捷方法
    void trace(const char* format, ...);
    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    
    // 宽字符版本
    void trace_w(const wchar_t* format, ...);
    void debug_w(const wchar_t* format, ...);
    void info_w(const wchar_t* format, ...);
    void warning_w(const wchar_t* format, ...);
    void error_w(const wchar_t* format, ...);
    
    // 启用/禁用控制台输出
    void enable_console(bool enable) { m_console_enabled = enable; }
    
    // 启用/禁用文件输出
    void enable_file(bool enable) { m_file_enabled = enable; }

private:
    Logger() = default;
    ~Logger() { shutdown(); }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void write_log(LogLevel level, const std::string& message);
    const char* level_to_string(LogLevel level);
    std::string get_timestamp();
    
    LogLevel m_level = LogLevel::INFO;
    std::wstring m_log_path;
    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_initialized = false;
    bool m_console_enabled = true;
    bool m_file_enabled = true;
};

// 便捷宏
#define LOG_TRACE(fmt, ...) hwid::Logger::instance().trace(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) hwid::Logger::instance().debug(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) hwid::Logger::instance().info(fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) hwid::Logger::instance().warning(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) hwid::Logger::instance().error(fmt, ##__VA_ARGS__)

} // namespace hwid

