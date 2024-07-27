#include "event/event_loop_thread.h"
#include "event/event_loop.h"

namespace event {

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb)
        : loop_(nullptr),
          thread_(std::bind(&EventLoopThread::ThreadFunction, this)),
          mutex_(),
          cond_(),
          is_existing_(false),
          callback_(cb) {
}

EventLoopThread::~EventLoopThread() {
    is_existing_ = true;
    if (loop_ != nullptr) {
        loop_->Quit();
        thread_.Join();
    }
}

EventLoop* EventLoopThread::StartLoop() {
    thread_.Start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::ThreadFunction() {
    EventLoop loop;
    if (callback_) callback_(&loop);

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop_->Loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

} // namespace event
