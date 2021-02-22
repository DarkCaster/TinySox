#ifndef CONFIG_H
#define CONFIG_H

#include "IConfig.h"
#include "IPAddress.h"
#include <string>
#include <vector>
#include <tuple>

class Config final : public IConfig
{
    private:
        std::vector<std::tuple<IPAddress,ushort>> listenAddrs;
        std::unordered_map<std::string,std::tuple<std::string,int>> users;
    public:
        void AddUser(const std::string &login, const std::string &password, const int level);
        void AddListenAddr(const IPAddress& addr, const ushort port);
        //from IConfig
        std::vector<std::tuple<IPAddress,ushort>> GetListenAddrs() const final;
        std::unordered_map<std::string,std::tuple<std::string,int>> GetUsers() const final;
};

#endif //CONFIG_H
