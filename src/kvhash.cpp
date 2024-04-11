#include <store/kvhash.hpp>

namespace zkv{
    
    int hash::string_hash(const std::string &kstr)
    {
        long long sum = 0;
        for(auto t: kstr){
            sum += (int)t;
        }
        
        return sum % size;
    }

    std::pair<hash::hashnode*, hash::hashnode* > hash::_get_key(std::string field)
    {
        auto idx = string_hash(field);

        auto q = dict[idx].next;
        auto* preq = &dict[idx];

        while(q){
            if(q->field == field)break;

            q = q->next;
            preq = preq->next;
            
        }

        return {preq,q};
    }

    hash::hash(){
        dict = new hashnode[size];
        if(!dict) throw std::runtime_error("ERROR NEW HASHDICT");
        memset(dict,0,sizeof(hashnode)*size);

        used = 0;
    }

    hash::~hash(){
        for(int i=0;i<size;i++){
            auto t = dict[i];
            auto p = t.next;

            while(p){
                auto pre = p;
                p = p->next;

                delete pre;
                pre = nullptr;
            }
        }

        delete[] dict;
        dict = nullptr;
        used = 0;
    }
}