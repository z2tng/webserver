#include "event/event_loop_thread_pool.h"
#include "event/event_loop_thread.h"
#include "event/event_loop.h"
#include "log/logger.h"

namespace event {

EventLoopThreadPool::EventLoopThreadPool(EventLoop *main_loop)
        : main_loop_(main_loop),
          is_started_(false),
          num_threads_(0),
          next_(0) {
    if (num_threads_ < 0) {
        LOG_ERROR << "Cannot create EventLoopThreadPool with " << num_threads_ << " threads";
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {
}

void EventLoopThreadPool::SetThreadNum(int num_threads) {
    num_threads_ = num_threads;
    sub_loop_threads_.reserve(num_threads_);
    sub_loops_.reserve(num_threads_);
}

void EventLoopThreadPool::Start(const ThreadInitCallback &cb) {
    is_started_ = true;

    if (num_threads_ == 0) {
        if (cb) cb(main_loop_);
        return;
    }

    for (int i = 0; i < num_threads_; ++i) {
        EventLoopThread *thread = new EventLoopThread(cb);
        sub_loop_threads_.push_back(std::unique_ptr<EventLoopThread>(thread));
        sub_loops_.push_back(thread->StartLoop());
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
