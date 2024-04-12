#include "store/kv.hpp"


namespace zkv{


    std::string kvstore::deal(std::string cmd)
    {
        debug(),"   deal with cmd: ",cmd;

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
                res = (num == 3)?
                set_string(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");

                break;

            case 1: //"GET KEY"
                res = (num == 2)?
                get_string(toks[1])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 2: //"DEL KEY"
                res = (num == 2)?
                del_string(toks[1])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 3: //"INC KEY"
                res = (num == 2)?
                inc_string(toks[1])
                : std::string("ERROR: CMD NUM ERROR!");
                break;
            
            case 4: //"DEC KEY"
                res = (num == 2)?
                dec_string(toks[1])
                : std::string("ERROR: CMD NUM ERROR!");
                break;
            
            case 5: //"RSET key score member [[score,member],...]"
                res = (num >= 4 && (num % 2 == 0))?
                rset(toks)
                : std::string("ERROR: CMD NUM ERROR!");
                break;
            
            case 6: //"RGET KEY begin end"
                switch(num){
                    case 4:
                        res = rget(toks[1],toks[2],toks[3]);
                        break;
                    case 3:
                        res = rget(toks[1],toks[2]);
                        break;
                    case 2:
                        res = rget(toks[1]);
                        break;
                    default:
                        res = std::string("ERROR: CMD NUM ERROR!");
                        break;
                }
                break;

            case 7: //RDEL KEY member
                res = (num == 3)?
                rdel(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 8: //"RINC KEY MEMBER"
                res = (num == 3)?
                rinc(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 9: //"RDEC KEY MEMBER"
                res = (num == 3)?
                rdec(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 10: //"RINCBY KEY MEMBER NUM"
                res = (num == 4)?
                rincby(toks[1],toks[2],toks[3])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 11: //"RDECBY KEY MEMBER NUM"
                res = (num == 4)?
                rdecby(toks[1],toks[2],toks[3])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 12: //"RSCORE KEY MEMBER"
                res = (num == 3)?
                rscore(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 13: //"RANK KEY MEMBER [ifback]"
                res = (num == 3 || num == 4)?
                rank(toks[1],toks[2],((num==4)?std::stoi(toks[3]):0))
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 14: //"HSET KEY FIELD VALUE [[FIELD VALUE]...]"
                res = ((num % 2 == 0) && (num >= 4))?
                hset(toks)
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 15: //"HGET KEY FIELD [FIELD...]"
                res = (num>=3)?
                hget(toks)
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 16: //"HDEL KEY FIELD"
                res = (num == 3)?
                hdel(toks[1],toks[2])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            case 17: //"HGETALL KEY"
                res = (num == 2)?
                hgetall(toks[1])
                : std::string("ERROR: CMD NUM ERROR!");
                break;

            default:
                res = "OK";
                break;
        }

        debug(),"       result of cmd: ",res;

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

    static bool checktype(kvtype thetype,kvnode* node){
        return node->type == thetype;
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
        for(int i=0;i<kvsize;i++){
            auto t = kvdict[i];
            auto pre = t.head->next;

            delete t.head;
            t.head = nullptr;

            while(pre){
                auto q = pre;
                pre = pre->next;

                switch(q->type){
                    case kvstring:
                        //string 型
                        delete (std::string*)q->data;
                        break;
                    case kvrset:
                        //rbtree 型
                        ((rbtree<ms>*)(q->data))->~rbtree();
                        delete ((rbtree<ms>*)(q->data));
                        break;
                    case kvhash:
                        break;
                    default:
                        break;
                }

                delete q;
                q = nullptr;
            }
        }
        if(kvdict)
            delete[] kvdict;

        kvdict = nullptr;

        used = 0;
        this->~kvstore();
        return;
    }

    int kvstore::HashString(const std::string &kstr)
    {
        long long sum = 0;
        for(auto t: kstr){
            sum += (int)t;
        }
        
        return sum % kvsize;
    }

    std::pair<kvnode *, kvnode *> kvstore::_get_key(std::string key)
    {
        auto idx = HashString(key);

        auto q = kvdict[idx].head->next;
        auto preq = kvdict[idx].head;

        while(q){
            if(q->key == key)break;

            q = q->next;
            preq = preq->next;
            
        }

        return {preq,q};
    }

    std::string kvstore::set_string(const std::string &key, const std::string &data)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;

        if(!q){ //没有 这个时候得新建节点
            auto node = new kvnode;
            node->type = kvstring;
            node->key = std::string(key);

            std::string* str = new std::string(data);
            node->data = (void*)str;

            node->next = nullptr;

            preq->next = node;
            return std::string("OK");
        } else {
            //找到了
            if(!checktype(kvstring,q))return std::string("SET ERROR: WRONG TYPE");
            *((std::string*)(q->data)) = std::string(data);
            return std::string("OK");
        }


        return std::string{};
    }

    std::string kvstore::del_string(const std::string& key)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("Error: No key named <")+key+">";
        else {
            if(!checktype(kvstring,q))return std::string("DEL ERROR: WRONG TYPE");
            preq->next = q->next;

            q->next = nullptr;

            if(q->data)
                delete (std::string*)q->data;
            q->data = nullptr;

            if(q)
                delete q;
            q = nullptr;
            return std::string("OK");
        }
        return 0;
    }

    std::string kvstore::get_string(const std::string &key)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
        

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvstring,q))return std::string("GET ERROR: WRONG TYPE");
            return std::string(*((std::string*)(q->data)));
        }

        return std::string();
    }

    static std::pair<bool,int> _stringtoint(const std::string& key){
        int res = 0;
        for(auto t:key){
            if(t < 48 || t > 57 ) return {false,0};
            res = res*10 + (t-48);
        }
        return {true,res};
    }

    std::string kvstore::inc_string(const std::string& key){
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvstring,q))return std::string("INC ERROR: WRONG TYPE");
            auto p = _stringtoint(*(std::string*)(q->data));
            if(p.first){
                int value = p.second;
                value++;
                *((std::string*)(q->data)) = std::to_string(value);
                return std::string("OK INC :")+std::string(*((std::string*)(q->data)));
            }
            else {
                return std::string("ERROR INC : not a number");
            }
        }
    }
    std::string kvstore::dec_string(const std::string& key){
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
        

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvstring,q))return std::string("DEC ERROR: WRONG TYPE");
            auto p = _stringtoint(*(std::string*)(q->data));
            if(p.first){
                int value = p.second;
                value--;
                *((std::string*)(q->data)) = std::to_string(value);
                return std::string("OK DEC :")+std::string(*((std::string*)(q->data)));
            }
            else {
                return std::string("ERROR DEC : not a number");
            }
        }
    }

    std::string kvstore::rset(const std::vector<std::string> &tokens)
    {   
        auto key = tokens[1];
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;

        if(!q){ //没有 这个时候得新建红黑树
            rbtree<ms>* newtree=new rbtree<ms>;

            auto node = new kvnode;
            node->type = kvrset;
            node->key = std::string(key);

            node->data = (void*)newtree;

            node->next = nullptr;

            preq->next = node;
            
            int insertnum = 0;
            for(int i=2;i<tokens.size();i+=2){

                auto sco = std::stoi(tokens[i]);
                auto mem = tokens[i+1];

                ((rbtree<ms>*)(node->data))->Insert(ms(mem,sco));
                insertnum++;
                
            }
            return std::string("OK")+" "+std::to_string(insertnum);
        } else {
            //找到了
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");
            
            auto tree = ((rbtree<ms>*)(q->data));
            int insertnum = 0;
            for(int i=2;i<tokens.size();i+=2){

                auto sco = std::stoi(tokens[i]);
                auto mem = tokens[i+1];
                auto check = tree->checkmember(mem);

                if(!check.first)
                    tree->Insert(ms(mem,sco));
                else {
                    tree->Remove(ms(mem,check.second));
                    tree->Insert(ms(mem,sco));
                }
                insertnum++;
                
            }
            return std::string("OK : ")+std::to_string(insertnum);
        }

        return std::string();
    }

    std::string kvstore::rdel(const std::string &key,const std::string& member)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;

        if(!q){ 
        return std::string("ERROR : NO THIS KEY");
        } else {
            //找到了
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");
            
            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");
            else tree->Remove(ms(member,check.second));

                

            return std::string("OK");
        }

        return std::string();
    }

    std::string kvstore::rget(const std::string& key,const std::string& begin,const std::string& end){

        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("FAIL:WRONG TYPE");
            
            auto tree = ((rbtree<ms>*)(q->data));
            int size = tree->size();
            int _begin = 0;
            int _end = 0;

            try {
                _begin = std::stoi(begin);
                _end = std::stoi(end);
            }
            catch(...){
                return std::string("ERROR:NOT A NUMBER");
            }

            if(_begin<0)_begin+=size;
            if(_end<0)_end+=size;

            std::vector<ms> res;

            if(_end>_begin)tree->kvInOrder(res,_begin,_end);
            else tree->kvInOrder(res,_end,_begin);

            std::string str = "";

            auto ressize = res.size();

            for(int i=0;i<ressize;i++){
                auto j=(_end>=_begin)?i:(ressize-i-1);
                str = str + "("+std::to_string(i+1)+") "+res[j].member+" "+std::to_string(res[j].score);
                str += '\n';
            }
            
            return str;
        }
        return std::string();
    }

    std::string kvstore::rinc(const std::string &key,const std::string &member)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");

            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");

            tree->Remove(ms(member,check.second));
            tree->Insert(ms(member,check.second+1));

            return std::string("RINC : ")+std::to_string(check.second+1);
            }
        
        return std::string();
    }

    std::string kvstore::rdec(const std::string &key,const std::string &member)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");

            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");

            tree->Remove(ms(member,check.second));
            tree->Insert(ms(member,check.second-1));

            return std::string("RDEC : ")+std::to_string(check.second-1);
            }
        
        return std::string();
    }

    std::string kvstore::rincby(const std::string &key,const std::string &member,const std::string& value)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");

            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");

            tree->Remove(ms(member,check.second));
            tree->Insert(ms(member,check.second+std::stoi(value)));

            return std::string("RINCBY : ")+std::to_string(check.second+std::stoi(value));
            }
        
        return std::string();
    }

    std::string kvstore::rdecby(const std::string &key,const std::string &member, const std::string& value)
    {
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");

            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");

            tree->Remove(ms(member,check.second));
            tree->Insert(ms(member,check.second-std::stoi(value)));

            return std::string("RDECBY : ")+std::to_string(check.second-std::stoi(value));
            }
        
        return std::string();
    }

    std::string kvstore::rscore(const std::string& key,const std::string& member){
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");

            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");

            return std::to_string(check.second);
            }
        
        return std::string();
    }

    std::string kvstore::rank(const std::string& key,const std::string& member,int ifback){
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q) return std::string("ERROR : NO THIS KEY");
        else {
            if(!checktype(kvrset,q))return std::string("ERROR: WRONG TYPE");

            auto tree = ((rbtree<ms>*)(q->data));

            auto check = tree->checkmember(member);
            
            if(!check.first)return std::string("ERROR: MEMBER NOT EXIST");
            tree->kvInOrder(member);
            auto rank = tree->getrank();

            if(ifback)rank = tree->size() - rank;

            return std::to_string(rank);
            }
        
        return std::string();
    }

    std::string kvstore::hset(const std::vector<std::string>& tokens){
        auto key = tokens[1];
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q){
            auto bignode = new kvnode;
            memset(bignode,0,sizeof(kvnode));
            bignode->type = kvhash;
            preq->next = bignode;

            //建新hash表
            hash* newhash = new hash;
            if(!newhash) throw std::runtime_error("ERROR NEW HASH");
            bignode->data = (void*)newhash;
            bignode->key = key;


            int dealnum = 0;
            for(int i=2;i<tokens.size();i+=2){
                auto field = tokens[i];
                auto value = tokens[i+1];

                auto location = newhash->_get_key(field);
                auto preloc = location.first;
                auto qloc = location.second;

                if(!qloc){
                    hash::hashnode* node = new hash::hashnode;
                    if(!node) throw std::runtime_error("ERROR NEW HASHNODE");


                    node->field = field;
                    node->value = value;
                    node->next = nullptr;
                    preloc->next = node;
                } else {
                    qloc->value = value;
                }
                
                dealnum++;
            }
            return std::string("OK ")+std::to_string(dealnum);
        }
        else {
            if(!checktype(kvhash,q))return std::string("ERROR: WRONG TYPE");
            int dealnum = 0;
            for(int i=2;i<tokens.size();i+=2){
                auto field = tokens[i];
                auto value = tokens[i+1];

                auto newhash = (hash*)(q->data);

                auto location = newhash->_get_key(field);
                auto preloc = location.first;
                auto qloc = location.second;

                if(!qloc){
                    hash::hashnode* node = new hash::hashnode;
                    if(!node) throw std::runtime_error("ERROR NEW HASHNODE");

                    node->field = field;
                    node->value = value;
                    node->next = nullptr;
                    preloc->next = node;
                } else {
                    qloc->value = value;
                }
                
                dealnum++;
            }
            return std::string("OK ")+std::to_string(dealnum);
            
        }
        return std::string();
    }

    std::string kvstore::hget(const std::vector<std::string>& tokens){
        auto key = tokens[1];
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q)return std::string("ERROR : NO THIS KEY");

        if(!checktype(kvhash,q))return std::string("ERROR: WRONG TYPE");

        auto ha = (hash*)(q->data);
        
        std::string res;
        
        for(int i=2;i<tokens.size();i++){
            res =  res + "("+std::to_string(i+1)+")";
            auto field = tokens[i];
            auto loc = ha->_get_key(field);
            auto locq = loc.second;

            if(!locq)res+=std::string("ERROR: NO THIS FIELD");
            else res+=locq->value;

            res+='\n';
        }

        return res;
    }

    std::string kvstore::hdel(const std::string& key,const std::string& field){
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q)return std::string("ERROR : NO THIS KEY");

        if(!checktype(kvhash,q))return std::string("ERROR: WRONG TYPE");

        auto ha = (hash*)(q->data);
        
        auto loc = ha->_get_key(field);
        auto preloc = loc.first;
        auto locq = loc.second;

        if(!locq)return std::string("ERROR: NO THIS FIELD");
        else {
            preloc->next = locq->next;
            delete locq;
            return std::string("OK");
        }

        return std::string();
    }

    std::string kvstore::hgetall(const std::string& key){
        auto pr = _get_key(key);
        auto preq = pr.first;
        auto q = pr.second;
       

        if(!q)return std::string("ERROR : NO THIS KEY");

        if(!checktype(kvhash,q))return std::string("ERROR: WRONG TYPE");

        auto ha = (hash*)(q->data);
        
        std::string res;
        
        for(int i=0,j=0;i < ha->size;i++){
            

            auto t = ha->dict[i].next;

            while(t){    
                res =  res + "("+std::to_string(++j)+")";
                auto field = t->field;
                auto value = t->value;

                res = res+field+" "+value;
                t = t->next;
                res+='\n';
            }
        }

        return res;
    }

}
