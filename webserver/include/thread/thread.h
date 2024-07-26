#pragma once

#include "utils/uncopyable.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <functional>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace current_thread {

thread_local int t_cached_tid;

inline int tid() {
    if (__builtin_expect(t_cached_tid == 0, 0)) {
        t_cached_tid = static_cast<int>(::syscall(SYS_gettid));
    }
    return t_cached_tid;
}

} // namespace current_thread

namespace thread
{

class Thread : utils::Uncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(const ThreadFunc &func, const std::string &name = std::string());
    ~Thread();

    void Start();
    void Join();

    bool is_started() const { return is_started_; }
    std::thread::id thread_id() const { return thread_id_; }
    std::string name() const { return name_; }

private:
    std::thread::id thread_id_;
    std::thread thread_;
    ThreadFunc func_;
    std::string name_;
    bool is_started_;
    bool is_joined_;
    std::once_flag flag_;

    static std::atomic_int started_count_;
};
    
} // namespace thread