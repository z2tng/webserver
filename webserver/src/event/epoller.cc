#include "event/epoller.h"
#include "event/channel.h"
#include "log/logger.h"

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
        LOG_FATAL << "epoll create error: " << errno;
    }
}

Epoller::~Epoller() {
    ::close(epoll_fd_);
}

void Epoller::Poll(ChannelList &active_channels) {
    int num_events = epoll_wait(epoll_fd_, &*ready_events_.begin(), ready_events_.size(), kEpollTimeOut);

    if (num_events < 0 && errno != EINTR) {
        LOG_ERROR << "epoll wait error: " << errno;
        return;
    } else if (num_events == 0) {
        LOG_ERROR << "epoll wait timeout";
        return;
    }

    for (int i = 0; i < num_events; ++i) {
        Channel *channel = static_cast<Channel*>(ready_events_[i].data.ptr);
        channel->set_revents(ready_events_[i].events);
        active_channels.push_back(channel);
    }

    if (num_events == ready_events_.size()) {
        ready_events_.resize(ready_events_.size() * 2);
    }
}

bool Epoller::HasChannel(Channel *channel) const {
    auto it = channel_map_.find(channel->fd());
    return it != channel_map_.end() && it->second == channel;
}

void Epoller::UpdateChannel(Channel *channel, int timeout) {
    int fd = channel->fd();
    if (channel_map_.find(fd) == channel_map_.end()) {
        channel_map_[fd] = channel;
        Update(EPOLL_CTL_ADD, channel);
    } else {
        if (channel->IsNoneEvent()) {
            Update(EPOLL_CTL_DEL, channel);
            channel_map_.erase(fd);
        } else {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::RemoveChannel(Channel *channel) {
    int fd = channel->fd();
    if (channel_map_.find(fd) != channel_map_.end()) {
        Update(EPOLL_CTL_DEL, channel);
        channel_map_.erase(fd);
    }
}

void Epoller::Update(int operation, Channel *channel) {
    epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();

    if (epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_ADD) {
            LOG_ERROR << "epoll_ctl add error: " << strerror(errno);
        } else if (operation == EPOLL_CTL_MOD) {
            LOG_ERROR << "epoll_ctl mod error: " << strerror(errno);
        } else {
            LOG_ERROR << "epoll_ctl del error: " << strerror(errno);
        }
    }
}

} // namespace event
