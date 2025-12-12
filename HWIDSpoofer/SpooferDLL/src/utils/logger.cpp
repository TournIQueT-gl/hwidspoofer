#include "logger.h"
#include <Windows.h>
#include <ctime>
#include <cstdio>

namespace hwid {

bool Logger::initialize(const std::wstring& log_path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    // 设置日志路径
    if (!log_path.empty()) {
        m_log_path = log_path;
        m_file.open(log_path, std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            m_file_enabled = false;
        }
    }
    
    // 分配控制台（如果调试模式）
#ifdef _DEBUG
    if (m_console_enabled) {
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        SetConsoleTitleW(L"HWID Spoofer Debug Console");
    }
#endif
    
    m_initialized = true;
    return true;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    if (m_file.is_open()) {
        m_file.close();
    }
    
#ifdef _DEBUG
    FreeConsole();
#endif
    
    m_initialized = false;
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    struct tm tm_info;
    localtime_s(&tm_info, &time);
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
            tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
            static_cast<int>(ms.count()));
    
    return buffer;
}

const char* Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERR:     return "ERROR";
        default:                return "?????";
    }
}

void Logger::write_log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (level < m_level) {
        return;
    }
    
    std::string timestamp = get_timestamp();
    std::string formatted = "[" + timestamp + "] [" + level_to_string(level) + "] " + message + "\n";
    
    // 输出到控制台
    if (m_console_enabled) {
        // 设置颜色
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        
        switch (level) {
            case LogLevel::TRACE:
                color = FOREGROUND_INTENSITY;
                break;
            case LogLevel::DEBUG:
                color = FOREGROUND_GREEN | FOREGROUND_BLUE;
                break;
            case LogLevel::INFO:
                color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case LogLevel::WARNING:
                color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case LogLevel::ERR:
                color = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
        }
        
        SetConsoleTextAttribute(hConsole, color);
        printf("%s", formatted.c_str());
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    
    // 输出到文件
    if (m_file_enabled && m_file.is_open()) {
        m_file << formatted;
        m_file.flush();
    }
    
    // 输出到调试器
    OutputDebugStringA(formatted.c_str());
}

void Logger::log(LogLevel level, const char* format, ...) {
    if (level < m_level) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log(level, buffer);
}

void Logger::log(LogLevel level, const wchar_t* format, ...) {
    if (level < m_level) return;
    
    wchar_t wbuffer[4096];
    va_list args;
    va_start(args, format);
    vswprintf(wbuffer, sizeof(wbuffer) / sizeof(wchar_t), format, args);
    va_end(args);
    
    // 转换为多字节
    char buffer[4096];
    WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, buffer, sizeof(buffer), nullptr, nullptr);
    
    write_log(level, buffer);
}

void Logger::trace(const char* format, ...) {
    if (LogLevel::TRACE < m_level) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log(LogLevel::TRACE, buffer);
}

void Logger::debug(const char* format, ...) {
    if (LogLevel::DEBUG < m_level) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log(LogLevel::DEBUG, buffer);
}

void Logger::info(const char* format, ...) {
    if (LogLevel::INFO < m_level) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log(LogLevel::INFO, buffer);
}

void Logger::warning(const char* format, ...) {
    if (LogLevel::WARNING < m_level) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log(LogLevel::WARNING, buffer);
}

void Logger::error(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log(LogLevel::ERR, buffer);
}

void Logger::trace_w(const wchar_t* format, ...) {
    if (LogLevel::TRACE < m_level) return;
    log(LogLevel::TRACE, format);
}

void Logger::debug_w(const wchar_t* format, ...) {
    if (LogLevel::DEBUG < m_level) return;
    log(LogLevel::DEBUG, format);
}

void Logger::info_w(const wchar_t* format, ...) {
    if (LogLevel::INFO < m_level) return;
    log(LogLevel::INFO, format);
}

void Logger::warning_w(const wchar_t* format, ...) {
    if (LogLevel::WARNING < m_level) return;
    log(LogLevel::WARNING, format);
}

void Logger::error_w(const wchar_t* format, ...) {
    log(LogLevel::ERR, format);
}

} // namespace hwid

