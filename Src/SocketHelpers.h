#ifndef SOCKET_HELPERS_H
#define SOCKET_HELPERS_H

#include "Config.h"
#include "ILogger.h"

#include <atomic>

class TCPSocketHelper
{
    private:
        ILogger &logger;
        const IConfig &config;
        const int fd;
        std::atomic<bool> &cancel;
        bool readAllowed;
        bool writeAllowed;
    public:
        TCPSocketHelper(ILogger &logger, const IConfig &config, const int fd, std::atomic<bool> &cancel);
        int ReadData(char * const target, const int len, const bool allowPartial);
        int WriteData(const char * const target, const int len);
};

#endif //SOCKET_HELPERS_H
