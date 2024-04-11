#include <debug.hpp>
#include <reactor.hpp>
#include <callback.hpp>

int main(){
    debug(),"hello world!";

    
    
    zkv::kvstore kv;
    kv.init_kvstore(-1);

    zkv::reactor t;
    t.Init_reactor();
    t.create_server(8888,zkv::listen_callback_kv,1,&kv);
    t.stop();

    return 0;
}