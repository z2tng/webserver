#include "thread/thread.h"

#include <semaphore.h>

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
    is_started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    thread_ = std::thread([&] {
        thread_id_ = std::this_thread::get_id();
        sem_post(&sem);
        func_();
    });

    sem_wait(&sem);
}

void Thread::Join() {
    is_joined_ = true;
    thread_.join();
}

}