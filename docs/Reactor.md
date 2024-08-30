# Reactor 模型：EventLoop、Channel 和 Poller

Reactor 模型是构建高性能 WebServer 的一种比较成熟的实现方式，相比于传统的阻塞 I/O 模型，Reactor 模型基于事件驱动能够更加高效地处理大量并发请求。

事件驱动模型的核心组件包括：**EventLoop**、**Poller** 和 **Channel**，本文将阐述这些组件在服务器中发挥的作用和具体实现方式。

## 引言

在高并发环境下，传统的阻塞 IO 模型通常因为线程切换和阻塞等待而造成性能瓶颈。事件驱动模型通过非阻塞 IO 操作和事件驱动来高效管理多个连接，提高系统的并发处理能力。通常会通过结合 IO 多路复用机制（`select`、`poll`、`epoll`）来实现高效的事件管理，本项目中就基于 `epoll` 实现 Poller 组件。下面将一次介绍三个核心组件。

## EventLoop

**职责**：`EventLoop` 主要负责管理事件循环、分发事件以及执行相应的回调操作。每个线程会有自己的 `EventLoop`，以避免线程间的数据竞争。

**实现细节**：

- `EventLoop` 会通过一个循环不断地检查是否有新的事件需要处理。通过调用 `Poller` 来获取就绪事件集合。
- 获取事件后，`EventLoop` 根据事件类型调用相应回调函数进行处理。典型回调函数包括（读回调、写回调、错误处理等）
- `EventLoop` 还管理着定时器任务，用于处理延迟操作或定时任务。

**伪代码示例**：

```C++
void EventLoop::loop() {
    while (!quit_) {
        std::vector<Channel*> active_channels;
        poller_->poll(active_channels);
        for (Channel* channel : active_channels) {
            channel->handleEvent();
        }
        doPendingTasks(); // 处理其他待处理任务
    }
}
```

## Channel

**职责**：`Channel` 是事件的抽象，封装了文件描述符(如套接字)和与其相关联的事件回调（连接、读、写、关闭等）。`Channel` 负责根据事件类型执行响应的回调函数。

**实现细节**：

- `Channel` 不直接持有事件，而是注册到 `EventLoop` 中的 `Poller` 组件。`Poller` 监听到事件发生时，会通知对应的 `Channel`。
- 每个 `Channel` 对应一个文件描述符，一个 `Channel` 只能被一个 `EventLoop` 持有。

**伪代码示例**：

```C++
class Channel {
public:
    void handleEvents() {
        // 根据revents_调用具体的事件处理回调
    }

    // 所有回调函数的 setter
    void SetReadCallback(const EventCallback& cb) { read_callack_ = cb;}
    // ...

private:
    int fd_; // 文件描述符
    int events_; // 关注的事件类型
    int revents_; // 实际发生的事件类型
    
    // 回调函数
    EventCallback read_callback_;
    // 其它回调函数...
};
```

除上述核心代码外，一般还提供更新(update)和移除(remove)函数，便于动态修改文件描述符感兴趣的事件和从 Poller 中移除。

## Poller

**职责**：`Poller` 是 `EventLoop` 和 `Channle` 之间的桥梁，负责实际的 IO 事件监听。它提供了一种机制来监控多个描述符的状态。常见的 `Poller` 实现包括 `select`、`poll` 和 `epoll`。在高性能服务器中，`epoll` 是更为常用的选择，因为它具有更好的扩展性的性能表现。

**实现细节**：

- `Poller` 维护一个内部数据结构来跟踪所有注册的文件描述符机器关联的事件。在项目中采用 `epoll` 实现，使用内核提供的 `epoll_wait` 来高效地等待事件发生。
- 事件发生时，`epoll_wait` 返回一组已经准备好的文件描述符，`Poller` 会通知对应的 `Channel` 处理这些事件。

> 若系统同时支持 poll 和 epoll，会将 Poller 实现为一个抽象基类，提供相应接口。

**伪代码示例**：

```C++
class Poller {
public:
    void Epoller::Poll(ChannelList &active_channels) {
        int num_events = epoll_wait(epoll_fd_, &*ready_events_.begin(), ready_events_.size(), kEpollTimeOut);
        
        // 当 num_events < 0 或 == 0 时，输出信息并return

        for (int i = 0; i < num_events; ++i) {
            Channel *channel = static_cast<Channel*>(ready_events_[i].data.ptr);
            channel->set_revents(ready_events_[i].events);
            active_channels.push_back(channel);
        }
    }

    void UpdateChannel(Channel* channel) {
        epoll_event event;
        bzero(&event, sizeof(event));
        event.events = channel->events();
        event.data.ptr = channel;

        int fd = channel->fd();
        if (channel_map_.find(fd) == channel_map_.end()) {
            channel_map_[fd] = channel;
            epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->fd(), &event);
        } else {
            if (channel->IsNoneEvent()) {
                epoll_ctl(epollfd_, EPOLL_CTL_DEL, channel->fd(), &event);
                channel_map_.erase(fd);
            } else {
                epoll_ctl(epollfd_, EPOLL_CTL_MOD, channel->fd(), &event);
            }
        }
    }

private:
    using ChannelMap = std::unordered_map<int, Channel*>;

    int epollfd_;
    std::vector<epoll_event> ready_events_;
    ChannelMap channel_map_;
};
```

## 联系

1. **EventLoop & Poller**：EventLoop 持有一个 Poller 对象，并在 loop 函数(事件循环)中调用 Poller 的 poll 方法阻塞地等待并获取发生的事件。一旦事件发生，EventLoop 将会遍历这些事件，并找到相应的 Channel 对象。
2. **EventLoop & Channel**：EventLoop 管理着一组注册到该 EventLoop 下的 Poller 中的 Channel 对象，每个 Channel 关联了一个文件描述符、感兴趣的事件以及相应的事件处理回调函数。当 Poller 检测到某个文件描述符上有事件发生时，EventLoop 调用相应 Channel 对象的事件处理函数，处理发生的事件。
3. **Poller & Channel**：Poller 负责监听所有注册到的 Channel 对象。当某个 Channel 关联的文件描述符上有事件发生时，Poller 会将这个 Channel 加入到活动 Channel 列表中，返回给 EventLoop 进行下一步处理。

> Poller 并不之间调用 Channel 对象的方法，而是交由 EventLoop 进行调用

## 总结

事件驱动模型通过 EventLoop、Channel 和 Poller 三个组件协同工作，实现了高效的事件管理和处理。EventLoop 负责管理事件循环和任务调度，Channel 作为事件的抽象封装与处理单元，而 Poller 则是连接 I/O 多路复用机制与应用逻辑的纽带。这种模型减少了线程切换的开销，并能高效地处理大量并发连接，是构建高性能 Web 服务器的理想选择。通过上述组件的设计和实现，可以显著提升服务器的性能和扩展性，为高并发的 Web 应用提供可靠的技术支持。