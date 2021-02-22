#include "Config.h"

void Config::AddUser(const std::string& login, const std::string& password, const int level)
{
    users[login]=std::tuple<std::string,int>(password,level);
}

void Config::AddListenAddr(const IPAddress &addr, const ushort port)
{
    listenAddrs.push_back(std::tuple<IPAddress,ushort>(addr,port));
}

std::vector<std::tuple<IPAddress,ushort>> Config::GetListenAddrs() const
{
    return listenAddrs;
}

std::unordered_map<std::string,std::tuple<std::string,int>> Config::GetUsers() const
{
    return users;
}
