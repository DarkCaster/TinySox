#ifndef ICONFIG_H
#define ICONFIG_H

#include "IPAddress.h"
#include "IPEndpoint.h"

#include "string"
#include "unordered_map"
#include "unordered_set"
#include <sys/time.h>

#include <tuple>

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
};

#endif
