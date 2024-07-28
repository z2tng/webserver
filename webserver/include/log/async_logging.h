#pragma once

#include "utils/uncopyable.h"
#include "log/log_stream.h"
#include "thread/thread.h"

#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>


namespace log {

class FileUtils {
public:
    explicit FileUtils(const std::string &file_name,
                       size_t buffer_size = 64 * 1024)
            : fp_(fopen(file_name.c_str(), "ae")) {
        buffer_.resize(buffer_size);
        setvbuf(fp_, &buffer_[0], _IOFBF, buffer_size);
    }

    ~FileUtils() {
        fclose(fp_);
    }

    void Write(const char *buffer, size_t size) {
        size_t write_size = fwrite_unlocked(buffer, 1, size, fp_);
        size_t remain = size - write_size;
        while (remain) {
            size_t n = fwrite_unlocked(buffer + write_size, 1, remain, fp_);
            if (n == 0) {
                int err = ferror(fp_);
                if (err) {
                    fprintf(stderr, "FileUtils::Write() failed: %d\n", err);
                    break;
                }
            }
            write_size += n;
            remain = size - write_size;
        }
    }

    void Flush() {
        fflush(fp_);
    }

private:
    FILE *fp_;
    std::vector<char> buffer_;
};

class LogFile : utils::Uncopyable {
public:
    LogFile(const std::string &file_name, int flush_interval = 1024)
            : file_name_(file_name),
              flush_interval_(flush_interval),
              count_(0),
              file_(new FileUtils(file_name)),
              mutex_() {
    }

    ~LogFile() {}

    void Write(const char *log, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_->Write(log, size);
        ++count_;
        if (count_ >= flush_interval_) {
            count_ = 0;
            Flush();
        }
    }

    void Flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        file_->Flush();
    }

private:
    const std::string file_name_;
    const int flush_interval_;
    int count_;
    std::mutex mutex_;
    std::unique_ptr<FileUtils> file_;
};

class AsyncLogging : utils::Uncopyable {
public:
    using Buffer = FixedBuffer<kLargeBufferSize>;

    AsyncLogging(const std::string &file_name, int timeout = 3000); // 3s
    ~AsyncLogging();

    void Write(const char *log, size_t size, bool is_fatal = false);
    void Start();
    void Stop();

private:
    using BufferList = std::vector<std::shared_ptr<Buffer>>;

    void ThreadFunc();

    static const int kMaxBufferCount = 24;

    std::string file_name_;
    const int timeout_;
    bool is_running_;

    std::shared_ptr<Buffer> current_buffer_;
    std::shared_ptr<Buffer> next_buffer_;
    BufferList buffers_;

    thread::Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

};

} // namespace log

