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
    using ChannelList = std::vector<std::shared_ptr<Channel>>;

    Epoller();
    ~Epoller();

    void Poll(ChannelList &active_channels);

    void EpollAdd(std::shared_ptr<Channel> sp_channel, int timeout);
    void EpollMod(std::shared_ptr<Channel> sp_channel, int timeout);
    void EpollDel(std::shared_ptr<Channel> sp_channel);

    int epoll_fd() const { return epoll_fd_; }
private:
    using ChannelMap = std::unordered_map<int, std::shared_ptr<Channel>>;

    static const int kEpollTimeOut = 10000;
    static const int kInitEventListSize = 16;

    void Update(int operation, std::shared_ptr<Channel> sp_channel);

    int epoll_fd_;
    std::vector<epoll_event> ready_events_;
    ChannelMap channel_map_;
};

} // namespace event
