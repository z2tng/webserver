#include "event/event_loop_thread_pool.h"
#include "event/event_loop.h"
#include "event/event_loop_thread.h"

namespace event {

EventLoopThreadPool::EventLoopThreadPool(EventLoop *main_loop, int num_threads)
        : main_loop_(main_loop),
          is_started_(false),
          num_threads_(num_threads),
          next_(0) {
    if (num_threads_ < 0) {
        // LOG
    }
    sub_loop_threads_.reserve(num_threads_);
    sub_loops_.reserve(num_threads_);
}

EventLoopThreadPool::~EventLoopThreadPool() {
}

void EventLoopThreadPool::Start(const ThreadInitCallback &cb) {
    is_started_ = true;

    if (num_threads_ == 0) {
        if (cb) cb(main_loop_);
        return;
    }

    for (int i = 0; i < num_threads_; ++i) {
        std::shared_ptr<EventLoopThread> thread(new EventLoopThread(cb));
        sub_loop_threads_.emplace_back(thread);
        sub_loops_.emplace_back(thread->StartLoop());
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
    EventLoop *loop = main_loop_;
    if (!sub_loops_.empty()) {
        loop = sub_loops_[next_];
        next_ = (next_ + 1) % num_threads_;
    }
    return loop;
}

    
} // namespace event
