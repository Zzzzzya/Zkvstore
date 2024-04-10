#include "callback.hpp"

namespace zkv {
    void send_callback_kv(int fd, int events, void *pri)
    {
        auto ev = (myevent*)pri;

        char target = '\n';
        size_t pos = ev->out.find(target);
        if (pos != std::string::npos) {
            std::string result = ev->out.substr(0, pos);
            //debug(),getLocalIPAddress(ev->fd)," :",result;
            ev->out = ev->out.erase(0,pos+1);

            write(ev->fd,result.c_str(),result.length());

            ev->r->motif_event(ev,EPOLLIN);
        }
    }
    void read_callback_kv(int fd, int events, void *pri)
    {
        auto ev = (myevent*)pri;
        int n = ev->r->readToBuffer(ev);
        if(n < 0)return;
        char target = '\n';
        size_t pos = ev->in.find(target);
        if (pos != std::string::npos) {
            std::string result = ev->in.substr(0, pos);
            //debug(),getLocalIPAddress(ev->fd)," :",result;
            ev->in = ev->in.erase(0,pos+1);


            ev->out.append(ev->db->deal(result)+"\n");
            
            ev->r->motif_event(ev,EPOLLOUT);
        }
        return;
    }
    void listen_callback_kv(int fd, int events, void *pri)
    {
        auto e = (myevent*)pri;

        struct sockaddr_in addr = {0};
        int len = sizeof(struct sockaddr_in);

        int clientfd = accept(fd,(struct sockaddr*)&addr,(socklen_t*)&len);
        checkerror(clientfd);

        char str[1024];
        debug(),"connect from ",inet_ntop(AF_INET, &addr.sin_addr, str, sizeof(str)),"port ",ntohs(addr.sin_port);

        auto ev = e->r->new_event(clientfd,read_callback_kv,send_callback_kv,nullptr,e->db);

        e->r->add_event(ev,EPOLLIN);
        zkv::setNonblock(clientfd);
        return;
    }
}