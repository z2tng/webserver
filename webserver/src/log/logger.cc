#include "log/logger.h"
#include "log/async_logging.h"
#include "thread/thread.h"

#include <memory>
#include <condition_variable>
#include <time.h>

namespace log {

std::string Logger::log_file_name_ = "./server.log";

static std::shared_ptr<AsyncLogging> async_logging;

void OnceInit() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        async_logging = std::make_shared<AsyncLogging>(Logger::GetLogFileName());
        if (async_logging) async_logging->Start();
    });
}

void WriteLog(const char *log, size_t size, bool is_fatal) {
    OnceInit();
    async_logging->Write(log, size, is_fatal);
}

Logger::Logger(const std::string &file_name, int line, int level)
        : impl_(file_name, line, level) {
    impl_.stream_ << impl_.time_ << ' '
                  << impl_.file_name_ << ':'
                  << impl_.line_ << impl_.level_str_;
}

Logger::~Logger() {
    impl_.stream_ << '\n';

    const auto &buffer = stream().buffer();
    WriteLog(buffer.buffer(), buffer.size(), impl_.is_fatal_);

    if (impl_.is_fatal_) abort();
}

Logger::Impl::Impl(const std::string &file_name, int line, int level)
        : stream_(),
          file_name_(file_name),
          line_(line),
          level_(level),
          is_fatal_(level == FATAL) {
    auto pos = file_name_.find_last_of('/');
    if (pos != std::string::npos) file_name_ = file_name_.substr(pos + 1);

    FormatTime();
    FormatLevel();
}

Logger::Impl::~Impl() {
}

void Logger::Impl::FormatTime() {
    int64_t microseconds(time(nullptr));
    tm *current_time = localtime(&microseconds);
    strftime(time_, sizeof(time_), "%Y-%m-%d %H:%M:%S", current_time);
}

void Logger::Impl::FormatLevel() {
    switch (level_) {
        case DEBUG:
            level_str_ = " [DEBUG]: ";
            break;
        case INFO:
            level_str_ = " [INFO]: ";
            break;
        case WARN:
            level_str_ = " [WARN]: ";
            break;
        case ERROR:
            level_str_ = " [ERROR]: ";
            break;
        case FATAL:
            level_str_ = " [FATAL]: ";
            break;
        default:
            level_str_ = " [INFO]: ";
            break;
    }
}

} // namespace log


