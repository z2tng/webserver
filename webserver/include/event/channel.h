#pragma once

#include <memory>
#include <functional>

namespace event {

class EventLoop;

class Channel : public std::enable_shared_from_this<Channel> {
public:
    using EventCallback = std::function<void()>;

    explicit Channel(EventLoop *loop, int fd);
    ~Channel();

    void HandleEvents();

    int fd() const { return fd_; }
    void set_fd(int fd) { fd_ = fd; }

    int events() const { return events_; }
    int last_events() const { return last_events_; }
    void set_revents(int revents) { revents_ = revents; }
    bool update_last_events() {
        // 如果注册的事件发生变化, 则返回 true
        bool ret = last_events_ != events_;
        last_events_ = events_;
        return ret;
    }

    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return events_ & kWriteEvent; }
    bool IsReading() const { return events_ & kReadEvent; }

    void EnableReading() { events_ |= kReadEvent; update(); }
    void EnableWriting() { events_ |= kWriteEvent; update(); }
    void DisableReading() { events_ &= ~kReadEvent; update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; update(); }
    void DisableAll() { events_ = kNoneEvent; update(); }

    void set_holder(std::shared_ptr<void> holder) { holder_ = holder; holded_ = true; }
    std::shared_ptr<void> holder() {
        std::shared_ptr<void> holder(holder_.lock());
        return holder;
    }

    void SetReadCallback(EventCallback &&cb) { read_callback_ = cb; }
    void SetWriteCallback(EventCallback &&cb) { write_callback_ = cb; }
    void SetCloseCallback(EventCallback &&cb) { close_callback_ = cb; }
    void SetErrorCallback(EventCallback &&cb) { error_callback_ = cb; }

    EventLoop* GetLoop() const { return loop_; }
    void Remove();

private:
    void HandleEventsWithGuard();
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    int fd_; // 文件描述符
    int events_; // 注册的事件
    int revents_; // epoll 返回的事件
    int last_events_; // 上一次注册的事件, 用于判断是否需要 epoll_ctl_mod

    std::weak_ptr<void> holder_;
    bool holded_;

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

} // namespace event