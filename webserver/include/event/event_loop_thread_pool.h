#pragma once

#include "utils/uncopyable.h"

#include <functional>
#include <memory>
#include <vector>

namespace event {

class EventLoop;
class EventLooopThread;

class EventLoopThreadPool : utils::Uncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *main_loop, int num_threads);
    ~EventLoopThreadPool();

    void Start(const ThreadInitCallback &cb = ThreadInitCallback());
    EventLoop* GetNextLoop();

private:
    using EventLoopThreadList = std::vector<std::shared_ptr<EventLooopThread>>;
    using EventLoopList = std::vector<EventLoop*>;

    EventLoop* main_loop_;
    bool is_started_;
    int num_threads_;
    int next_;
    EventLoopThreadList sub_loop_threads_;
    EventLoopList sub_loops_;
};
    
} // namespace event
