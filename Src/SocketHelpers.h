#ifndef SOCKET_HELPERS_H
#define SOCKET_HELPERS_H

#include "Config.h"
#include "ILogger.h"
#include "SocketClaim.h"

#include <atomic>
#include <vector>
#include <memory>
#include <sys/epoll.h>

class TCPSocketReader
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig &config;
        const int fd;
        const int epollFd;
        std::shared_ptr<std::atomic<bool>> cancel;
        int readState;
        epoll_event sockEvents;
    public:
        TCPSocketReader(std::shared_ptr<ILogger> &logger, const IConfig &config, const int fd, std::shared_ptr<std::atomic<bool>> &cancel);
        ~TCPSocketReader();
        int ReadData(unsigned char * const target, const int len, const bool allowPartial);
        int GetReadState(); //-2 - cancelled, -1 - error, read will fail, 0 - read will block, 1 - read will not block, may read some data
        void Shutdown();
};

class TCPSocketWriter
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig &config;
        const int fd;
        const int epollFd;
        std::shared_ptr<std::atomic<bool>> cancel;
        int writeState;
        epoll_event sockEvents;
    public:
        TCPSocketWriter(std::shared_ptr<ILogger> &logger, const IConfig &config, const int fd, std::shared_ptr<std::atomic<bool>> &cancel);
        ~TCPSocketWriter();
        int WriteData(const unsigned char * const target, const int len);
        int GetWriteState(); //-2 - cancelled, -1 - error, write will fail, 0 - write will block, 1 - write may proceed
        void Shutdown();
};

class SocketClaimsCleaner
{
    public:
        static bool CloseUnclaimedSockets(std::shared_ptr<ILogger> &logger, const std::vector<SocketClaimState> &claimStates);
};

#endif //SOCKET_HELPERS_H
