#include "event/channel.h"

#include <sys/epoll.h>

namespace event {

Channel::Channel() : fd_(-1), events_(0), revents_(0), holded_(false) {}

Channel::Channel(int fd) : fd_(fd), events_(0), revents_(0), holded_(false) {}

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
        events_ = 0;
        return;
    }

    if (revents_ & EPOLLERR) {
        if (error_callback_) error_callback_();
        events_ = 0;
        return;
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) read_callback_();
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) write_callback_();
    }

    if (update_callback_) update_callback_();
}

}