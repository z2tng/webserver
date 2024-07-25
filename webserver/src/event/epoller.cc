#include "event/epoller.h"
#include "event/channel.h"

#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

namespace event
{

Epoller::Epoller()
        : epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
          ready_events_(kInitEventListSize) {
    if (epoll_fd_ < 0) {
        // TODO: LOG epoll create error
    }
}

Epoller::~Epoller() {
    ::close(epoll_fd_);
}

void Epoller::Poll(ChannelList &active_channels) {
    int num_events = epoll_wait(epoll_fd_, &*ready_events_.begin(), ready_events_.size(), kEpollTimeOut);

    if (num_events < 0 && errno != EINTR) {
        // TODO: LOG error
        return;
    } else if (num_events == 0) {
        // TODO: LOG timeout
        return;
    }

    for (int i = 0; i < num_events; ++i) {
        Channel *channel = static_cast<Channel*>(ready_events_[i].data.ptr);
        channel->set_revents(ready_events_[i].events);
        active_channels.push_back(channel->shared_from_this());
    }

    if (num_events == ready_events_.size()) {
        ready_events_.resize(ready_events_.size() * 2);
    }
}

void Epoller::EpollAdd(std::shared_ptr<Channel> sp_channel, int timeout) {
    // TODO: 处理超时
    int fd = sp_channel->fd();
    channel_map_[fd] = sp_channel;
    sp_channel->update_last_events();
    Update(EPOLL_CTL_ADD, sp_channel);
}

void Epoller::EpollMod(std::shared_ptr<Channel> sp_channel, int timeout) {
    // TODO: 处理超时
    if (sp_channel->update_last_events()) {
        Update(EPOLL_CTL_MOD, sp_channel);
    }
}

void Epoller::EpollDel(std::shared_ptr<Channel> sp_channel) {
    channel_map_.erase(sp_channel->fd());
    Update(EPOLL_CTL_DEL, sp_channel);
}

void Epoller::Update(int operation, std::shared_ptr<Channel> sp_channel) {
    epoll_event event;
    bzero(&event, sizeof(event));
    event.events = operation == EPOLL_CTL_DEL ?
                       sp_channel->last_events() : sp_channel->events();
    event.data.ptr = sp_channel.get();
    int fd = sp_channel->fd();

    if (epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_ADD) {
            // TODO: LOG add error
        } else if (operation == EPOLL_CTL_MOD) {
            // TODO: LOG mod error
        } else {
            // TODO: LOG del error
        }
    }
}

} // namespace event
