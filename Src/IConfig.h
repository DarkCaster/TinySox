#ifndef ICONFIG_H
#define ICONFIG_H

#include "IPAddress.h"
#include "vector"
#include "string"
#include "unordered_map"
#include <tuple>

class IConfig
{
    public:
        virtual std::vector<std::tuple<IPAddress,ushort>> GetListenAddrs() const = 0;
        virtual std::unordered_map<std::string,std::tuple<std::string,int>> GetUsers() const = 0;
};

#endif
