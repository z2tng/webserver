#pragma once

#include <memory>
#include <functional>

namespace event {

class EventLoop;

class Channel {
public:
    using EventCallback = std::function<void()>;

    explicit Channel(EventLoop *loop, int fd);
    ~Channel();

    void HandleEvents();

    int fd() const { return fd_; }
    void set_fd(int fd) { fd_ = fd; }

    int events() const { return events_; }
    void set_revents(int revents) { revents_ = revents; }

    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return events_ & kWriteEvent; }
    bool IsReading() const { return events_ & kReadEvent; }

    void EnableReading() { events_ |= kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }
    void DisableReading() { events_ &= ~kReadEvent; Update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    void set_holder(const std::shared_ptr<void> &holder) {
        holder_ = holder;
        holded_ = true;
    }

    void SetReadCallback(EventCallback &&cb) { read_callback_ = cb; }
    void SetWriteCallback(EventCallback &&cb) { write_callback_ = cb; }
    void SetCloseCallback(EventCallback &&cb) { close_callback_ = cb; }
    void SetErrorCallback(EventCallback &&cb) { error_callback_ = cb; }

    EventLoop* GetLoop() const { return loop_; }
    void Remove();

private:
    void HandleEventsWithGuard();
    void Update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    int fd_; // 文件描述符
    int events_; // 注册的事件
    int revents_; // epoll 返回的事件

    std::weak_ptr<void> holder_;
    bool holded_;

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

} // namespace event