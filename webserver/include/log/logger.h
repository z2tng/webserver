#pragma once

#include "log/log_stream.h"

#include <string>

namespace logging {

class Logger {
public:
    Logger(const std::string &file_name, int line, int level);
    ~Logger();

    LogStream& stream() { return impl_.stream_; }

    static void SetLogFileName(const std::string &file_name) {
        log_file_name_ = file_name;
    }
    static std::string GetLogFileName() { return log_file_name_; }

private:
    class Impl {
    public:
        Impl(const std::string &file_name, int line, int level);
        ~Impl();

        void FormatTime();
        void FormatLevel();

        LogStream stream_;
        std::string file_name_;
        int line_;
        int level_;
        std::string level_str_;
        char time_[32];
        bool is_fatal_;
    };

    static std::string log_file_name_;
    Impl impl_;
};

} // namespace log


enum LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL
};

#define LOG_INFO logging::Logger(__FILE__, __LINE__, INFO).stream()
#define LOG_WARN logging::Logger(__FILE__, __LINE__, WARN).stream()
#define LOG_ERROR logging::Logger(__FILE__, __LINE__, ERROR).stream()
#define LOG_FATAL logging::Logger(__FILE__, __LINE__, FATAL).stream()

#ifdef ON_DEBUG
#define LOG_DEBUG log::Logger(__FILE__, __LINE__, DEBUG).stream()
#else
#define LOG_DEBUG false && logging::Logger(__FILE__, __LINE__, DEBUG).stream()
#endif
