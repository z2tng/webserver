#pragma once

#include "utils/uncopyable.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>


namespace event {

class Channel;

class Epoller : utils::Uncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Epoller();
    ~Epoller();

    void Poll(ChannelList &active_channels);

    bool HasChannel(Channel *channel) const;

    void UpdateChannel(Channel *channel, int timeout);
    void RemoveChannel(Channel *channel);

    int epoll_fd() const { return epoll_fd_; }
private:
    using ChannelMap = std::unordered_map<int, Channel*>;

    static const int kEpollTimeOut = 10000;
    static const int kInitEventListSize = 16;

    void Update(int operation, Channel *p_channel);

    int epoll_fd_;
    std::vector<epoll_event> ready_events_;
    ChannelMap channel_map_;
};

} // namespace event
