#pragma once

#include "utils/uncopyable.h"
#include "thread/thread.h"

#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace event {

class EventLoop;

class EventLoopThread : utils::Uncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback());
    ~EventLoopThread();

    std::shared_ptr<EventLoop> StartLoop();

private:
    void ThreadFunction();

    std::shared_ptr<EventLoop> loop_;
    thread::Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool is_existing_;
    ThreadInitCallback callback_;
};

} // namespace event
