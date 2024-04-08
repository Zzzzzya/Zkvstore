#include "reactor.hpp"

namespace zkv{
void reactor::Init_reactor() { 
    epfd = epoll_create(666);
    checkerror(epfd);

    listenfd = 0;

    evpool = new myevent[MAX_EVENT_RING];
    epoll_ev = new struct epoll_event[MAX_EPOLL_EVENT];

    memset(evpool,0,sizeof(myevent)*MAX_EVENT_RING);
    memset(epoll_ev,0,sizeof(struct epoll_event)*MAX_EPOLL_EVENT);
    return;
}

void reactor::destroy_reactor() {
    close(epfd);
    if(listenfd) close(listenfd);
    delete[] evpool;
    delete[] epoll_ev;
    return ;
}

myevent* reactor::new_event(int fd,
 event_callback_fd rd, 
 event_callback_fd wr,
 error_callback_fd er) 
 {
    assert(!rd || !wr || !er);

    iter++;
    int count = 0;
    // 从reactor中分配好的event池子里面找到一个位置拿取
    while(evpool[iter].fd != 0){
        if(++iter >= MAX_EVENT_RING) [[unlikely]] iter = 0;
        if(++count > MAX_EVENT_RING) [[unlikely]] return nullptr;
    }

    myevent* ev = &evpool[iter];
    ev->r = this;
    ev->fd = fd;
    ev->rd = rd;
    ev->wr = wr;
    ev->err = er;

    return ev;
}

void reactor::destroy_event(myevent *ptr) {
    ptr->fd = 0;
    ptr->rd = nullptr;
    ptr->wr = nullptr;
    ptr->err = nullptr;
    return;
}


int reactor::add_event(myevent *ev, int mask) { 
    auto r = ev->r;

    struct epoll_event aev;
    aev.data.ptr = ev;
    aev.events = mask;

    int ret = epoll_ctl(r->epfd,EPOLL_CTL_ADD,ev->fd,&aev);
    checkerror(ret);

    return 0; 
}

int reactor::delete_event(myevent *ev) { 

    int ret = epoll_ctl(ev->r->epfd,EPOLL_CTL_DEL,ev->fd,nullptr);
    checkerror(ret);

    return 0; 
}

int reactor::motif_event(myevent *ev, int mask) { 

    struct epoll_event aev;
    aev.data.ptr = ev;
    aev.events = mask;

    int ret = epoll_ctl(ev->r->epfd,EPOLL_CTL_MOD,ev->fd,&aev);
    checkerror(ret);

    return 0; 
}

void reactor::looponce() {
    ssize_t nev = epoll_wait(epfd,epoll_ev,MAX_EPOLL_EVENT,0);
    for(int i=0;i<nev;i++){
        auto ev = epoll_ev[i];
        auto pri = (myevent*)ev.data.ptr;
        int mask = ev.events;

        if(ev.events & EPOLLERR) mask |= EPOLLIN;
        if(ev.events & EPOLLHUP) mask |= EPOLLIN;

        if(ev.events & EPOLLIN){
            if(pri->rd)
                pri->rd(pri->fd,EPOLLIN,pri);
        } 
        else if(ev.events & EPOLLOUT){
            if(pri->wr)
                pri->wr(pri->fd,EPOLLOUT,pri);
        }
    }
}

void reactor::run() {
    while(!stop_flag){
        looponce();
    }
    return ;
}

void reactor::create_server(short port, event_callback_fd listen_callback,bool ifrun) {
    //首先绑定套接字
    assert(listen_callback != nullptr);

    listenfd = socket(AF_INET,SOCK_STREAM,0);
    checkerror(listenfd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = 0000;

    int reuse = 0;
    checkerror((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int))));

    checkerror(bind(listenfd,(struct sockaddr*)&addr,(socklen_t)sizeof(struct sockaddr)));

    checkerror(listen(listenfd,LISTEN_NUM));

    auto ev = new_event(listenfd,listen_callback,nullptr,nullptr);

    add_event(ev,EPOLLIN);
    debug(),"listen port :",port;

    if(ifrun)
        run();

    return;
}

void reactor::stop() {
    stop_flag = 1;
    return;
}

int reactor::readToBuffer(myevent* ev){
    //读入缓冲区。读至“\n”
    int num = 0;
    while(1){
        int rem = ev->inRemain();
        char buffer[1024] = {0};
        int n = read(ev->fd,buffer,rem);
        if(n == 0){
            //断联
            debug(),"断开连接：",ev->fd;
            if(ev->err)
                ev->err(ev->fd,"connection break");
            delete_event(ev);
            close(ev->fd);
            return -1;
        }

        if(n < 0){
            if(errno == EINTR)continue;
            if(errno == EAGAIN)break;

            debug(),"error ! ",strerror(errno);
            if(ev->err)
                ev->err(ev->fd,strerror(errno));
            delete_event(ev);
            close(ev->fd);
            return -2;
        }

        debug(),"recv from socket:",ev->fd," len:",n;
        ev->in.append(buffer);
        num+=n;
    }
    return num;
}

void checkerror(int ret){
    if(ret != -1) return ;
    throw std::runtime_error(strerror(errno));
}

int setNonblock(int fd) { 
        int flag = fcntl(fd, F_GETFL, 0);
        return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

std::string getLocalIPAddress(int sockfd) {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        int ret = getsockname(sockfd, (struct sockaddr*)&addr, &addrlen); // 获取套接字的本地地址信息
        if (ret == -1) {
            perror("getsockname");
            return "";
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN); // 将地址转换为字符串形式

        return std::string(ip);
}

void read_callback_default(int fd,int events,void* pri){
    auto ev = (myevent*)pri;
    int n = ev->r->readToBuffer(ev);
    if(n < 0)return;
    char target = '\n';
    size_t pos = ev->in.find(target);
    if (pos != std::string::npos) {
        std::string result = ev->in.substr(0, pos);
        debug(),getLocalIPAddress(ev->fd)," :",result;
        ev->in = ev->in.erase(0,pos+1);
    }
    return;
}

void listen_callback_default(int fd,int events,void* pri){

    auto e = (myevent*)pri;

    struct sockaddr_in addr = {0};
    int len = sizeof(struct sockaddr_in);

    int clientfd = accept(fd,(struct sockaddr*)&addr,(socklen_t*)&len);
    checkerror(clientfd);

    char str[1024];
    debug(),"connect from ",inet_ntop(AF_INET, &addr.sin_addr, str, sizeof(str)),"port ",ntohs(addr.sin_port);

    auto ev = e->r->new_event(clientfd,read_callback_default,nullptr,nullptr);

    e->r->add_event(ev,EPOLLIN);
    zkv::setNonblock(clientfd);
    return;
}

} // namespace zkv