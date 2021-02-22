#include "Config.h"

void Config::AddListenAddr(const IPEndpoint &endpoint)
{
    listenAddrs.insert(endpoint);
}

void Config::SetSocketTimeoutMS(int timeoutMS)
{
    socketTimeout=timeoutMS;
}

void Config::SetServiceIntervalMS(int intervalMS)
{
    serviceInterval=intervalMS;
}

void Config::SetWorkersCount(int count)
{
    workersCount=count;
}

void Config::SetWorkersSpawnCount(int count)
{
    workersSpawnCount=count;
}

int Config::GetSocketTimeoutMS() const
{
    return socketTimeout;
}

timeval Config::GetSocketTimeoutTV() const
{
    return timeval{socketTimeout/1000,(socketTimeout-socketTimeout/1000*1000)*1000};
}

int Config::GetServiceIntervalMS() const
{
    return serviceInterval;
}

timeval Config::GetServiceIntervalTV() const
{
    return timeval{serviceInterval/1000,(serviceInterval-serviceInterval/1000*1000)*1000};
}

int Config::GetWorkersCount() const
{
    return workersCount;
}

int Config::GetWorkersSpawnCount() const
{
    return workersSpawnCount;
}

std::unordered_set<IPEndpoint> Config::GetListenAddrs() const
{
    return listenAddrs;
}
