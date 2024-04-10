#pragma once
#include "debug.hpp"

#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include <unistd.h>
#include <string>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>

/*
    网络模块 使用epoll封装为reactor
*/
#define MAX_EVENT_RING ((1<<16)-1)
#define MAX_EPOLL_EVENT 1024
#define LISTEN_NUM 66

namespace zkv{
    //两大核心类 事件封装和reactor封装
    struct kvstore;
    struct myevent;
    struct reactor;

    //自封装回调函数， 分为事件处理（读写处理）&& 错误处理
    typedef void (*event_callback_fd)(int fd,int event,void *pridata);
    typedef void (*error_callback_fd)(int fd,std::string err); 

    struct myevent{
        int fd = -1;
        reactor* r = nullptr;

        event_callback_fd rd = nullptr;
        event_callback_fd wr = nullptr;
        error_callback_fd err = nullptr;
        
        //读写缓冲
        std::string in;
        std::string out;

        kvstore* db;
        

        int inRemain(){return 1024-in.length();}
        int outRemain(){return 1024-out.length();}
    };

    struct reactor{
        int epfd = -1;
        int listenfd = 0;
        int stop_flag = 0;

        myevent* evpool = nullptr;
        int iter = 0;

        struct epoll_event* epoll_ev = nullptr;

        reactor(){};
        reactor(reactor&&) = delete;
        reactor& operator=(reactor&&) = delete;
        ~reactor(){
            destroy_reactor();
        }

        void Init_reactor();
        void destroy_reactor();
        void looponce();
        void run();

        myevent* new_event(int fd,
        event_callback_fd rd,
        event_callback_fd wr,
        error_callback_fd er,
        kvstore* db = nullptr);

        void destroy_event(myevent* ptr);

        int add_event(myevent* ev,int mask);
        int delete_event(myevent* ev);
        int motif_event(myevent* ev,int mask);

        void create_server(short port,event_callback_fd listen_callback,bool run,kvstore* kv);

        void stop();

        int readToBuffer(myevent*);
        
        
    };

    void read_callback_default(int fd,int events,void* pri);
    void listen_callback_default(int fd,int events,void* pri);
    void checkerror(int);

    int setNonblock(int fd);

    std::string getLocalIPAddress(int sockfd);
    
}