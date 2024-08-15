#include "thread/thread.h"

namespace current_thread {

thread_local int t_cached_tid = 0;
    
} // namespace current


namespace thread {

std::atomic_int Thread::started_count_(0);

Thread::Thread(const ThreadFunc &func, const std::string &name)
        : thread_id_(0),
          func_(func),
          name_(name),
          is_started_(false),
          is_joined_(false) {
    int num = ++started_count_;
    if (name.empty()) {
        name_ = "Thread" + std::to_string(num);
    }
}

Thread::~Thread() {
    if (is_started_ && !is_joined_) {
        thread_.detach();
    }
}

void Thread::Start() {
    std::call_once(flag_, [this] {
        is_started_ = true;
        thread_id_ = std::this_thread::get_id();
        func_();
    });
}

void Thread::Join() {
    is_joined_ = true;
    thread_.join();
}

}