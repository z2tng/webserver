#pragma once

#include "utils/uncopyable.h"
#include "thread/thread.h"
#include "epoller.h"

#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

class Channel;
class Epoller;

namespace event
{

class EventLoop : utils::Uncopyable {
public:
    using Function = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void Loop();
    void Quit();

    void Wakeup();

    void RunInLoop(const Function &func);
    void QueueInLoop(const Function &func);

    bool HasChannel(std::shared_ptr<Channel> channel) const {
        return epoller_->HasChannel(channel);
    }

    void PollerAdd(std::shared_ptr<Channel> channel, int timeout = 0) {
        epoller_->EpollAdd(channel, timeout);
    }
    void PollerMod(std::shared_ptr<Channel> channel, int timeout = 0) {
        epoller_->EpollMod(channel, timeout);
    }
    void PollerDel(std::shared_ptr<Channel> channel) {
        epoller_->EpollDel(channel);
    }

    bool is_in_loop_thread() const { return thread_id_ == current_thread::tid(); }

private:
    static int CreateEventFd();
    void HandleRead();
    void PerformPendingFunctions();

    using ChannelList = std::vector<std::shared_ptr<Channel>>;

    const pid_t thread_id_;
    int wakeup_fd_;
    std::shared_ptr<Channel> wakeup_channel_;

    std::shared_ptr<Epoller> epoller_;
    ChannelList active_channels_;

    std::mutex mutex_;
    std::vector<Function> pending_functions_;

    std::atomic_bool is_looping_;
    std::atomic_bool is_quit_;
    std::atomic_bool is_handling_;
    std::atomic_bool is_calling_pending_functions_;
};
    
} // namespace event

