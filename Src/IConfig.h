#ifndef ICONFIG_H
#define ICONFIG_H

#include "IPEndpoint.h"
#include "User.h"
#include "udns.h"

#include "string"
#include "unordered_set"
#include <sys/time.h>

class IConfig
{
    public:
        virtual std::unordered_set<IPEndpoint> GetListenAddrs() const = 0;
        //various timeouts and intervals
        virtual int GetSocketTimeoutMS() const = 0;
        virtual timeval GetSocketTimeoutTV() const = 0;
        virtual int GetServiceIntervalMS() const = 0;
        virtual timeval GetServiceIntervalTV() const = 0;
        //settings for job dispatcher
        virtual int GetWorkersCount() const = 0;
        virtual int GetWorkersSpawnCount() const = 0;
        virtual int GetActiveWorkersCount() const = 0;
        virtual const User * GetUser(const std::string &name) const = 0;
        virtual dns_ctx * GetBaseUDNSContext() const = 0;
        virtual bool GetUDNSSearchDomainIsSet() const = 0;
        virtual int GetTCPBuffSz() const = 0;
        virtual int GetLingerSec() const = 0;
        virtual int GetMinCTimeSec() const = 0;
        virtual int GetMaxCTimeSec() const = 0;
        virtual int GetRWTimeoutSec() const = 0;
        virtual int GetHalfCloseTimeoutSec() const = 0;
        virtual std::string GetNetNS() const = 0;
};

#endif
