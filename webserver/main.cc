#include <iostream>

#include "http/http_server.h"
#include "log/logger.h"
#include "memory/memory_pool.h"
#include "cache/lfu_cache.h"

int main() {
    logging::Logger::SetLogFileName("./webserver.log");
    memory::InitMemoryPool();
    cache::LfuCache::instance().Init(10);

    event::EventLoop loop;
    net::InetAddress addr(5000);
    http::HttpServer http_server(&loop, addr);

    http_server.SetThreadNum(3);
    http_server.Start();
    loop.Loop();
    return 0;
}