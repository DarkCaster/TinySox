#ifndef SOCKET_HELPERS_H
#define SOCKET_HELPERS_H

#include "ILogger.h"
#include "IConfig.h"

#include <sys/time.h>
#include <memory>

class SocketHelpers
{
    public:
        static void TuneSocketBaseParams(std::shared_ptr<ILogger> &logger, int fd, const IConfig &config);
        static void SetSocketNonBlocking(std::shared_ptr<ILogger> &logger, int fd);
        static void SetSocketDefaultTimeouts(std::shared_ptr<ILogger> &logger, int fd, const IConfig &config);
        static void SetSocketCustomTimeouts(std::shared_ptr<ILogger> &logger, int fd, const timeval &tv);
};

#endif //SOCKET_HELPERS_H
