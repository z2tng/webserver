#include "log/async_logging.h"

namespace logging {

AsyncLogging::AsyncLogging(const std::string &file_name, int timeout)
        : file_name_(file_name),
          timeout_(timeout),
          is_running_(false),
          current_buffer_(new Buffer),
          next_buffer_(new Buffer),
          buffers_(),
          thread_(std::bind(&AsyncLogging::ThreadFunc, this)),
          mutex_(),
          cond_() {
    buffers_.reserve(16);
}

AsyncLogging::~AsyncLogging() {
    if (is_running_) Stop();
}

void AsyncLogging::Write(const char *log, size_t size, bool is_fatal) {
    std::lock_guard<std::mutex> lock(mutex_);
    {
        if (current_buffer_->capacity() > size) {
            current_buffer_->write(log, size);
            if (is_fatal) {
                Stop();
                abort();
            }
        } else {
            buffers_.push_back(current_buffer_);
            current_buffer_.reset();
            if (next_buffer_) {
                current_buffer_ = std::move(next_buffer_);
            } else {
                current_buffer_.reset(new Buffer);
            }

            current_buffer_->write(log, size);
            if (is_fatal) {
                Stop();
                abort();
            }
            // 通知后台线程有数据需要写入文件
            cond_.notify_one();
        }
    }
}

void AsyncLogging::Start() {
    is_running_ = true;
    thread_.Start();
}

void AsyncLogging::Stop() {
    is_running_ = false;
    cond_.notify_one();
    thread_.Join();
}

void AsyncLogging::ThreadFunc() {
    LogFile log_file(file_name_);
    std::shared_ptr<Buffer> new_buffer1 = std::make_shared<Buffer>();
    std::shared_ptr<Buffer> new_buffer2 = std::make_shared<Buffer>();
    BufferList buffers_to_write;
    buffers_to_write.reserve(16);

    while (is_running_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty()) {
                // 等待超时或者有数据需要写入
                cond_.wait_for(lock, std::chrono::microseconds(timeout_));
            }
            // 将当前 buffer 写入文件
            buffers_.push_back(std::move(current_buffer_));
            current_buffer_ = std::move(new_buffer1);
            buffers_to_write.swap(buffers_);
            if (!next_buffer_) {
                next_buffer_ = std::move(new_buffer2);
            }
        }

        // 当缓冲区过多时, 删除多余的缓冲区
        if (buffers_to_write.size() >= kMaxBufferCount) {
            buffers_to_write.erase(buffers_to_write.begin() + 2, buffers_to_write.end());
        }

        for (const auto &buffer : buffers_to_write) {
            log_file.Write(buffer->buffer(), buffer->size());
        }

        // 重用 buffer
        if (!new_buffer1) {
            new_buffer1 = buffers_to_write.back();
            buffers_to_write.pop_back();
            new_buffer1->clear(); // 重置 buffer, 以便下次使用
        }

        if (!new_buffer2) {
            new_buffer2 = buffers_to_write.back();
            buffers_to_write.pop_back();
            new_buffer2->clear();
        }

        buffers_to_write.clear();
        log_file.Flush();
    }
    log_file.Flush();
}

} // namespace log

