#pragma once
#include <debug.hpp>
#include <exception>
#include <string>
#include <cstring>

namespace zkv{
    
    struct hash{
        struct hashnode{
            std::string field;
            std::string value;
            hashnode* next;
        };

        const int size = 64;
        hashnode* dict;
        int used;

        hash();
        ~hash();
        int string_hash(const std::string &kstr);
        std::pair<hashnode*, hashnode* > _get_key(std::string field);

        
    };
}