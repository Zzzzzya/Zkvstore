#pragma once

#include <string>
#include "store/MurmurHash3.hpp"
#include <exception>
#include <debug.hpp>
#include <vector>
#include <cstring>
#include <assert.h>
/*
    key - value 的存储结构
    使用哈希字典 + 链表
*/

namespace zkv{

    void strtok(std::string, char op, std::vector<std::string> &res);

    struct cmdset{
        std::vector<std::string> cmdset = {
            "SET", "GET", "DEL", "MOD", "COUNT",
            "RSET", "RGET", "RDEL", "RMOD", "RCOUNT",
            "HSET", "HGET", "HDEL", "HMOD", "HCOUNT"
        };

        int find(std::string input){
            auto size = cmdset.size();
            for(int i=0;i<size-1;i++){
                if(input == cmdset[i])return i;
            }
            return -1;
        }
    };

    struct kvnode{
        std::string key;
        void* data;
        kvnode* next;
    };

    struct kvstore{
        const int kvsize = (1024);

        struct kvstore_node{
            kvnode* head;
            std::string type;
        };

        kvstore_node* kvdict = nullptr;
        int used = 0;

        kvstore(){};
        ~kvstore(){};

        std::string deal(std::string cmd);
        

        void init_kvstore(int size);
        void destroy_kvstore();

    private:
        std::string set_string(const std::string& key,const std::string& data);
        int del_string(const std::string& key,const std::string& data);
        std::string get_string(const std::string& key);

        
        int mymurmurHashString(const std::string& str);
    };    
    

}