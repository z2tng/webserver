#include "event/event_loop.h"
#include "event/channel.h"
#include "event/epoller.h"
#include "thread/thread.h"
#include "log/logger.h"

#include <unistd.h>
#include <sys/eventfd.h>
#include <fcntl.h>

namespace event {

__thread EventLoop *t_loop_in_this_thread = nullptr;

int EventLoop::CreateEventFd() {
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        LOG_ERROR << "Create eventfd error";
    }
    return event_fd;
}

EventLoop::EventLoop()
        : thread_id_(current_thread::tid()),
          wakeup_fd_(CreateEventFd()),
          wakeup_channel_(new Channel(this, wakeup_fd_)),
          epoller_(new Epoller()),
          is_looping_(false),
          is_quit_(false),
          is_handling_(false),
          is_calling_pending_functions_(false) {
    LOG_INFO << "EventLoop created " << this << " in thread " << thread_id_;
    if (t_loop_in_this_thread) {
        LOG_FATAL << "Another EventLoop " << t_loop_in_this_thread
                  << " exists in this thread " << thread_id_;
    } else {
        t_loop_in_this_thread = this;
    }
    wakeup_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop() {
    ::close(wakeup_fd_);
    wakeup_channel_->DisableAll();
    wakeup_channel_->Remove();
    t_loop_in_this_thread = nullptr;
}

void EventLoop::Loop() {
    is_looping_ = true;
    is_quit_ = false;

    while (!is_quit_) {
        active_channels_.clear();
        epoller_->Poll(active_channels_);
        for (auto &channel : active_channels_) {
            channel->HandleEvents();
        }
        PerformPendingFunctions();
    }

    is_looping_ = false;
}

void EventLoop::Quit() {
    is_quit_ = true;
    if (!is_in_loop_thread()) {
        Wakeup();
    }
}

void EventLoop::RunInLoop(const Function &func) {
    if (is_in_loop_thread()) {
        func();
    } else {
        QueueInLoop(func);
    }
}

void EventLoop::QueueInLoop(const Function &func) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pending_functions_.emplace_back(func);
    }

    if (!is_in_loop_thread() || is_calling_pending_functions_) {
        Wakeup();
    }
}

void EventLoop::Wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::Wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::HandleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::PerformPendingFunctions() {
    std::vector<Function> functions;
    is_calling_pending_functions_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functions.swap(pending_functions_);
    }

    for (const Function &func : functions) {
        func();
    }
    is_calling_pending_functions_ = false;
}

} // namespace event
