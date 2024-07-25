#include "event/event_loop.h"
#include "event/channel.h"
#include "event/epoller.h"
#include "thread/thread.h"

#include <unistd.h>
#include <sys/eventfd.h>
#include <fcntl.h>


namespace event {

__thread EventLoop *t_loop_in_this_thread = nullptr;

int EventLoop::CreateEventFd() {
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        // TODO: LOG create eventfd error
    }
    return event_fd;
}

EventLoop::EventLoop()
        : thread_id_(current_thread::tid()),
          wakeup_fd_(CreateEventFd()),
          wakeup_channel_(new Channel()),
          epoller_(new Epoller()),
          is_looping_(false),
          is_quit_(false),
          is_handling_(false),
          is_calling_pending_functions_(false) {
    
    // TODO: LOG create EventLoop in thread
    if (t_loop_in_this_thread) {
        // TODO: error, one loop per thread
    }
    wakeup_channel_->set_fd(wakeup_fd_);
    wakeup_channel_->set_read_callback(std::bind(&EventLoop::HandleRead, this));
}

EventLoop::~EventLoop() {
    ::close(wakeup_fd_);
    epoller_->EpollDel(wakeup_channel_);
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
    int n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        //LOG wakeup error
    }
}

void EventLoop::HandleRead() {
    uint64_t one = 1;
    int n = read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        //LOG wakeup error
    }
}

void EventLoop::HandleUpdate() {
    PollerMod(wakeup_channel_);
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
