#include "deal.hpp"
#include <assert.h>

void zkv::strtok(std::string cmd, char op, std::vector<std::string> &res)
{
    assert(res.empty());

    auto pos = cmd.find(op,0);
    auto prepos = pos - pos;

    while(pos != std::string::npos){
        res.push_back(cmd.substr(prepos, pos));
        prepos = pos+1;
        pos = cmd.find(op,prepos);
    }

    res.push_back(cmd.substr(prepos,cmd.length()-1));
    return ;
}

std::string zkv::Dealer::deal(std::string cmd)
{
    std::vector<std::string> res;
    zkv::strtok(cmd, ' ',res);
    
    for(auto t:res){
        debug(),t;
    }

    return std::string("OK\n");
}
