#include "SocketHelpers.h"

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

TCPSocketReader::TCPSocketReader(std::shared_ptr<ILogger> &_logger, const IConfig &_config, const int _fd, std::shared_ptr<std::atomic<bool>> &_cancel):
    logger(_logger),
    config(_config),
    fd(dup(_fd)),
    epollFd(epoll_create(1)),
    cancel(_cancel)
{
    readState=0;
    sockEvents={};
    sockEvents.events=EPOLLIN|EPOLLRDHUP;
    if(fd<0)
    {
        logger->Error()<<"TCPSocketReader: dup(fd) failed: "<<strerror(errno);
        readState=-1;
    }
    if(epollFd<0)
    {
        logger->Error()<<"TCPSocketReader: Failed to create epoll fd: "<<strerror(errno);
        readState=-1;
    }
    else if(epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&sockEvents)<0)
    {
        logger->Error()<<"TCPSocketReader: Failed to setup epoll: "<<strerror(errno);
        readState=-1;
    }
}

TCPSocketWriter::TCPSocketWriter(std::shared_ptr<ILogger> &_logger, const IConfig &_config, const int _fd, std::shared_ptr<std::atomic<bool>> &_cancel):
    logger(_logger),
    config(_config),
    fd(dup(_fd)),
    epollFd(epoll_create(1)),
    cancel(_cancel)
{
    writeState=0;
    sockEvents={};
    sockEvents.events=EPOLLOUT;
    if(fd<0)
    {
        logger->Error()<<"TCPSocketWriter: dup(fd) failed: "<<strerror(errno);
        writeState=-1;
    }
    if(epollFd<0)
    {
        logger->Error()<<"TCPSocketWriter: Failed to create epoll fd: "<<strerror(errno);
        writeState=-1;
    }
    else if(epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&sockEvents)<0)
    {
        logger->Error()<<"TCPSocketWriter: Failed to setup epoll: "<<strerror(errno);
        writeState=-1;
    }
}

TCPSocketReader::~TCPSocketReader()
{
    if(epollFd>=0 && epoll_ctl(epollFd,EPOLL_CTL_DEL,fd, &sockEvents)<0)
        logger->Error()<<"TCPSocketReader: Failed to deconfigure epoll: "<<strerror(errno);
    if(epollFd>=0 && close(epollFd)<0)
        logger->Error()<<"TCPSocketReader: Failed to close epoll fd: "<<strerror(errno);
    if(epollFd>=0 && close(fd)<0)
        logger->Error()<<"TCPSocketReader: Failed to close socket fd(dup): "<<strerror(errno);
}


TCPSocketWriter::~TCPSocketWriter()
{
    if(epollFd>=0 && epoll_ctl(epollFd,EPOLL_CTL_DEL,fd, &sockEvents)<0)
        logger->Error()<<"TCPSocketWriter: Failed to deconfigure epoll: "<<strerror(errno);
    if(epollFd>=0 && close(epollFd)<0)
        logger->Error()<<"TCPSocketWriter: Failed to close epoll fd: "<<strerror(errno);
    if(epollFd>=0 && close(fd)<0)
        logger->Error()<<"TCPSocketWriter: Failed to close socket fd(dup): "<<strerror(errno);
}

int TCPSocketReader::ReadData(unsigned char * const target, const int len, const bool allowPartial)
{
    if(readState<0)
        return readState;

    auto dataLeft=len;
    while(true)
    {
        if(readState==0)
            while(GetReadState()==0){}
        if(readState<0)
        {
            if(readState==-2)
            {
                logger->Warning()<<"TCPSocketReader: Reading cancelled";
                linger cLinger={1,0}; //to save time on shutdown
                if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
                    logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno)<<std::endl;
            }
            return readState;
        }

        //read data
        auto dataRead=read(fd,reinterpret_cast<void*>(target+len-dataLeft),dataLeft);
        if(dataRead==0)
        {
            readState=-1;
            return readState;
        }
        if(dataRead<0)
        {
            auto error=errno;
            if(error!=EINTR)
                logger->Warning()<<"TCPSocketReader: Read ended with error: "<<strerror(error);
            readState=-1;
            return readState;
        }
        //reset read state on successful reads
        if(readState>0)
            readState=0;
        dataLeft-=static_cast<int>(dataRead);
        if(allowPartial)
            return static_cast<int>(dataRead);
        //return if we have read all the data requested
        if(dataLeft<1)
            return len;
    }
    return -1;
}

int TCPSocketWriter::WriteData(const unsigned char* const target, const int len)
{
    if(writeState<0)
        return writeState;
    if(writeState==0)
        while(GetWriteState()==0){}
    if(writeState<0)
    {
        if(writeState==-2)
        {
            logger->Warning()<<"TCPSocketWriter: Writing cancelled";
            linger cLinger={1,0}; //to save time on shutdown
            if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
                logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno)<<std::endl;
        }
        return writeState;
    }

    //write data and check for errors
    auto dataWritten=write(fd,reinterpret_cast<const void*>(target),len);
    if(dataWritten==0)
    {
        writeState=-1;
        return writeState;
    }
    if(dataWritten<len)
    {
        auto error=errno;
        if(error!=EINTR)
            logger->Warning()<<"TCPSocketWriter: Writing ended with error: "<<strerror(error);
        writeState=-1;
    }
    if(writeState>0) //reset write state on fully completed writes
        writeState=0;
    return static_cast<int>(dataWritten);
}

int TCPSocketReader::GetReadState()
{
    if(readState<0)
        return readState;

    if(cancel->load())
    {
        readState = -2;
        return readState;
    }

    epoll_event evt={};
    auto rv=epoll_wait(epollFd,&evt,1,config.GetSocketTimeoutMS());

    if(rv<0)
    {
        auto error=errno;
        if(error!=EINTR)
            logger->Info()<<"TCPSocketReader: GetReadState failed: "<<strerror(error);
        rv=-1;
    }
    else if(rv>0 && (evt.events&EPOLLIN)==0)
    {
        long state=evt.events;
        logger->Info()<<"TCPSocketReader: GetReadState: shutdown or error detected, epoll flags: "<<std::hex<<state;
        rv=-1;
    }

    readState=rv;
    return readState;
}

void TCPSocketReader::Shutdown()
{
    if(readState<-1)
        return;
    if(shutdown(fd,SHUT_RD)<0)
        logger->Info()<<"TCPSocketReader: Socket shutdown failed: "<<strerror(errno);
    readState=-2;
}

int TCPSocketWriter::GetWriteState()
{
    if(writeState<0)
        return writeState;

    if(cancel->load())
    {
        writeState = -2;
        return -2;
    }

    epoll_event evt={};
    auto rv=epoll_wait(epollFd,&evt,1,config.GetSocketTimeoutMS());

    if(rv<0)
    {
        auto error=errno;
        if(error!=EINTR)
            logger->Info()<<"TCPSocketWriter: GetWriteState failed: "<<strerror(error);
        rv=-1;
    }
    else if(rv>0 && (evt.events&EPOLLOUT)==0)
    {
        long state=evt.events;
        logger->Info()<<"TCPSocketWriter: GetWriteState: shutdown or error detected, epoll flags: "<<std::hex<<state;
        rv=-1;
    }

    writeState=rv;
    return rv;
}

void TCPSocketWriter::Shutdown()
{
    if(writeState<-1)
        return;
    if(shutdown(fd,SHUT_WR)<0)
        logger->Info()<<"TCPSocketWriter: Socket shutdown failed: "<<strerror(errno);
    writeState=-2;
}


bool SocketHelpers::CloseUnclaimedSockets(std::shared_ptr<ILogger> &logger, const std::vector<SocketClaimState> &claimStates)
{
    bool result=true;
    for(auto &cState:claimStates)
        if(cState.counter<1)
        {
            if(close(cState.socketFD)!=0)
            {
                logger->Error()<<"Failed to perform socket close: "<<strerror(errno);
                result=false;
            }
            else
                logger->Info()<<"Socket closed: "<<cState.socketFD;
        }
    return result;
}

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

void SocketHelpers::SetSocketFastShutdown(std::shared_ptr<ILogger> &logger, int fd)
{
    linger cLinger={1,0}; //to save time on shutdown
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
        logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno);
}

