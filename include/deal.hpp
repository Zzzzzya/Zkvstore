#pragma once
#include <string>
#include <vector>
#include "debug.hpp"

namespace zkv{

    void strtok(std::string, char op, std::vector<std::string> &res);

    struct Dealer{
        
        std::string deal(std::string cmd);

    };

}