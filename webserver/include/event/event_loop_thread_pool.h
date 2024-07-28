#pragma once

#include "utils/uncopyable.h"

#include <functional>
#include <memory>
#include <vector>

namespace event {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : utils::Uncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *main_loop, int num_threads);
    ~EventLoopThreadPool();

    void Start(const ThreadInitCallback &cb = ThreadInitCallback());
    EventLoop* GetNextLoop();

private:
    EventLoop* main_loop_;
    bool is_started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> sub_loop_threads_;
    std::vector<EventLoop*> sub_loops_;
};
    
} // namespace event
