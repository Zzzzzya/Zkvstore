#include "store/kv.hpp"


namespace zkv{


    std::string kvstore::deal(std::string cmd)
    {
        std::vector<std::string> toks;
        zkv::strtok(cmd, ' ',toks);
        
        auto idx = cmdset().find(toks[0]);
        auto num = toks.size();
        std::string res;

        switch(idx){
            case -1 : 
                debug(),"error: unknown command";
                return std::string("ERROR:unknown command!: ")+toks[0];
            
            case 0: //"SET KEY VALUE"
                debug(),"it is set cmd";
                res = (num == 3)?
                set_string(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");

                break;

            case 1: //"GET KEY"
                debug(),"it is get cmd";
                res = (num == 2)?
                get_string(toks[1])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            default:
                res = "OK";
                break;
        }

        return res;
    }

    void strtok(std::string cmd, char op, std::vector<std::string> &res)
    {
        assert(res.empty());
        while(cmd.back() == ' ') cmd.pop_back();
        
        

        auto pos = cmd.find(op,0);
        auto prepos = pos - pos;

        while(pos != std::string::npos){
            res.push_back(cmd.substr(prepos, pos - prepos));
            prepos = pos+1;
            pos = cmd.find(op,prepos);
        }

        res.push_back(cmd.substr(prepos,cmd.length()));
        return ;
    }

    void kvstore::init_kvstore(int size)
    {
        if(size > 0){
            *(int*)&kvsize = size;
        }
        
        used = 0;

        kvdict = new kvstore_node[kvsize];
        if(!kvdict){
            debug(),"kvdict new error!";
            throw std::runtime_error("kvdict new error!");
        }
        memset(kvdict,0,sizeof(kvstore_node)*kvsize);

        for(int i=0;i<kvsize;i++){
            kvdict[i].head = new kvnode;
            if(kvdict[i].head == nullptr){
                throw std::runtime_error("ERROR: NEW kvdict");
            }
            memset(kvdict[i].head,0,sizeof(kvnode));
        }

        

        return ;
    }

    void kvstore::destroy_kvstore()
    {
        if(kvdict)
            delete[] kvdict;

        //TODO: 完善析构 释放所有空间。 记得打type

        kvdict = nullptr;

        used = 0;
        this->~kvstore();
        return;
    }

    int kvstore::mymurmurHashString(const std::string &kstr)
    {
        long long sum = 0;
        for(auto t: kstr){
            sum += (int)t;
        }
        
        return sum % kvsize;
    }

    std::string kvstore::set_string(const std::string& key,const std::string& data)
    {
        auto idx = mymurmurHashString(key);

        auto q = kvdict[idx].head->next;
        auto preq = kvdict[idx].head;

        for(;q;q = q->next){
            if(preq != q) preq = preq->next;
            if(q->key == key)break;
        }

        if(!q){ //没有 这个时候得新建节点
            auto node = new kvnode;
            node->key = std::string(key);

            std::string* str = new std::string(data);
            node->data = (void*)str;

            node->next = nullptr;

            preq->next = node;
            return std::string("SET OK");
        } else {
            //找到了
            *((std::string*)(q->data)) = std::string(data);
            return std::string("SET OK : MOTIF");
        }


        return std::string{};
    }

    int kvstore::del_string(const std::string& key,const std::string& data)
    {
        return 0;
    }

    std::string kvstore::get_string(const std::string &key)
    {
        auto idx = mymurmurHashString(key);

        auto q = kvdict[idx].head->next;

        for(;q;q = q->next){
            if(q->key == key)break;
        }

        if(!q) return std::string("Nil");
        else return std::string(*((std::string*)(q->data)));

        return std::string();
    }
}