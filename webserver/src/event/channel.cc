#include "event/channel.h"
#include "event/event_loop.h"

#include <sys/epoll.h>

namespace event {

Channel::Channel(EventLoop *loop, int fd)
        : loop_(loop), fd_(fd), events_(0), revents_(0), holded_(false) {}

Channel::~Channel() {}

void Channel::HandleEvents() {
    if (holded_) {
        std::shared_ptr<void> guard = holder();
        if (guard) {
            HandleEventsWithGuard();
        }
    } else {
        HandleEventsWithGuard();
    }
}

void Channel::HandleEventsWithGuard() {
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (close_callback_) close_callback_();
    }

    if (revents_ & EPOLLERR) {
        if (error_callback_) error_callback_();
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) read_callback_();
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) write_callback_();
    }
}

void Channel::update() {
    loop_->PollerMod(shared_from_this());
}

void Channel::Remove() {
    loop_->PollerDel(shared_from_this());
}

}