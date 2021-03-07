#include "SocketHelpers.h"

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

void SocketHelpers::TuneSocketBaseParams(std::shared_ptr<ILogger> &logger, int fd, const IConfig& config)
{
    //set linger
    linger cLinger={0,0};
    cLinger.l_onoff=config.GetLingerSec()>=0;
    cLinger.l_linger=cLinger.l_onoff?config.GetLingerSec():0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
        logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno);
    //set buffer size
    auto bsz=config.GetTCPBuffSz();
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bsz, sizeof(bsz)))
        logger->Warning()<<"Failed to set SO_SNDBUF option to socket: "<<strerror(errno);
    bsz=config.GetTCPBuffSz();
    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bsz, sizeof(bsz)))
        logger->Warning()<<"Failed to set SO_RCVBUF option to socket: "<<strerror(errno);
}

void SocketHelpers::SetSocketNonBlocking(std::shared_ptr<ILogger>& logger, int fd)
{
    auto flags = fcntl(fd,F_GETFL,0);
    //set socket to be non-blocking
    if(flags<0)
        logger->Warning()<<"Failed to get socket flags fcntl: "<<strerror(errno);
    else if(fcntl(fd, F_SETFL, flags | O_NONBLOCK)<0)
        logger->Warning()<<"Failed to set socket flags fcntl: "<<strerror(errno);
}

void SocketHelpers::SetSocketDefaultTimeouts(std::shared_ptr<ILogger> &logger, int fd, const IConfig &config)
{
    timeval tv{config.GetRWTimeoutSec(),0};
    SetSocketCustomTimeouts(logger,fd,tv);
}

void SocketHelpers::SetSocketCustomTimeouts(std::shared_ptr<ILogger> &logger, int fd, const timeval &tv)
{
    timeval rtv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv))!=0)
        logger->Warning()<<"Failed to set SO_RCVTIMEO option to socket: "<<strerror(errno);
    timeval stv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv))!=0)
        logger->Warning()<<"Failed to set SO_SNDTIMEO option to socket: "<<strerror(errno);
}



