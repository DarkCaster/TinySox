#ifndef SOCKET_HELPERS_H
#define SOCKET_HELPERS_H

#include "Config.h"
#include "ILogger.h"
#include "SocketClaim.h"

#include <atomic>
#include <vector>
#include <memory>

class TCPSocketHelper
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig &config;
        const int fd;
        std::shared_ptr<std::atomic<bool>> cancel;
        bool readAllowed;
        bool writeAllowed;
    public:
        TCPSocketHelper(std::shared_ptr<ILogger> &logger, const IConfig &config, const int fd, std::shared_ptr<std::atomic<bool>> &cancel);
        int ReadData(unsigned char * const target, const int len, const bool allowPartial);
        int WriteData(const unsigned char * const target, const int len);
};

class SocketClaimsCleaner
{
    public:
        static bool CloseUnclaimedSockets(std::shared_ptr<ILogger> &logger, const std::vector<SocketClaimState> &claimStates);
};

#endif //SOCKET_HELPERS_H
