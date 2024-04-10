#pragma once
#include "reactor.hpp"
#include "debug.hpp"
#include "store/kv.hpp"

namespace zkv{

    void send_callback_kv(int fd,int events,void* pri);
    void read_callback_kv(int fd,int events,void* pri);
    void listen_callback_kv(int fd,int events,void* pri);
}