#pragma once

#include <string>
#include <exception>
#include <debug.hpp>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <assert.h>
#include <store/kvrbtree.hpp>
#include <store/kvhash.hpp>

/*
    key - value 的存储结构
    使用哈希字典 + 链表
*/

namespace zkv{

    void strtok(std::string, char op, std::vector<std::string> &res);

    struct cmdset{
        std::vector<std::string> cmdset = {
            "SET", "GET", "DEL", "INC","DEC",
            "RSET", "RGET", "RDEL", "RINC", "RDEC","RINCBY","RDECBY","RSCORE","RANK",
            "HSET", "HGET", "HDEL", "HGETALL"
        };

        std::vector<std::string> cmdsetlow = {
            "set", "get", "del", "inc","dec",
            "rset", "rget", "rdel", "rinc", "rdec","rincby","rdecby","rscore","rank",
            "hset", "hget", "hdel", "hgetall"
        };

        int find(std::string input){
            auto size = cmdset.size();
            for(int i=0;i<size-1;i++){
                if(input == cmdset[i] || input == cmdsetlow[i])return i;
            }
            return -1;
        }
    };

    typedef enum{
            kvstring = 0,
            kvrset,
            kvhash
        }kvtype;

    struct kvnode{
        std::string key;
        void* data;
        kvtype type;
        kvnode* next;
    };

    struct kvstore{
        const int kvsize = (1024);
        
        struct kvstore_node{
            kvnode* head;
        };

        kvstore_node* kvdict = nullptr;
        int used = 0;
        int used_string = 0;
        int used_hash = 0;
        int used_zset = 0;

        kvstore(){};
        ~kvstore(){};

        std::string deal(std::string cmd);
        

        void init_kvstore(int size);
        void destroy_kvstore();

    private:
        std::pair<kvnode*,kvnode*> _get_key(std::string key);

        std::string set_string(const std::string& key,const std::string& data);
        std::string del_string(const std::string& key);
        std::string get_string(const std::string& key);
        std::string inc_string(const std::string& key);
        std::string dec_string(const std::string& key);

        std::string rset(const std::vector<std::string>& tokens);
        std::string rdel(const std::string& key,const std::string& member);
        std::string rget(const std::string& key,const std::string& begin = "0",
                        const std::string& end = "-1");
        std::string rinc(const std::string& key,const std::string& member);
        std::string rdec(const std::string& key,const std::string& member);
        std::string rincby(const std::string& key,const std::string& member,const std::string& value);
        std::string rdecby(const std::string& key,const std::string& member,const std::string& value);
        std::string rscore(const std::string& key,const std::string& member);
        std::string rank(const std::string& key,const std::string& member,int ifback);

        std::string hset(const std::vector<std::string>& tokens);
        std::string hget(const std::vector<std::string>& tokens);
        std::string hdel(const std::string& key,const std::string& field);
        std::string hgetall(const std::string& key);

        int HashString(const std::string& str);
    };    
}