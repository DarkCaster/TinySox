#ifndef CONFIG_H
#define CONFIG_H

#include "IConfig.h"
#include "IPEndpoint.h"
#include "User.h"

#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

class Config final : public IConfig
{
    private:
        int socketTimeout;
        int serviceInterval;
        int workersCount;
        int workersSpawnCount;
        dns_ctx * context;
        std::unordered_set<IPEndpoint> listenAddrs;
        std::unordered_map<std::string,const User> users;
    public:
        void AddListenAddr(const IPEndpoint &endpoint);
        void AddUser(const User &user);
        void SetSocketTimeoutMS(int timeoutMS);
        void SetServiceIntervalMS(int intervalMS);
        void SetWorkersCount(int count);
        void SetWorkersSpawnCount(int count);
        void SetBaseUDNSContext(dns_ctx * context);
        //from IConfig
        std::unordered_set<IPEndpoint> GetListenAddrs() const final;
        int GetSocketTimeoutMS() const final;
        timeval GetSocketTimeoutTV() const final;
        int GetServiceIntervalMS() const final;
        timeval GetServiceIntervalTV() const final;
        int GetWorkersCount() const final;
        int GetWorkersSpawnCount() const final;
        const User * GetUser(const std::string &name) const final;
        dns_ctx * GetBaseUDNSContext() const final;
};

#endif //CONFIG_H
