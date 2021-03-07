#include "TCPCommService.h"
#include "TCPCommHelper.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

static void TuneSocketBaseParams(std::shared_ptr<ILogger> &logger, int fd, const IConfig& config)
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

static void SetSocketNonBlocking(std::shared_ptr<ILogger>& logger, int fd)
{
    auto flags = fcntl(fd,F_GETFL,0);
    //set socket to be non-blocking
    if(flags<0)
        logger->Warning()<<"Failed to get socket flags fcntl: "<<strerror(errno);
    else if(fcntl(fd, F_SETFL, flags | O_NONBLOCK)<0)
        logger->Warning()<<"Failed to set socket flags fcntl: "<<strerror(errno);
}

static void SetSocketCustomTimeouts(std::shared_ptr<ILogger> &logger, int fd, const timeval &tv)
{
    timeval rtv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv))!=0)
        logger->Warning()<<"Failed to set SO_RCVTIMEO option to socket: "<<strerror(errno);
    timeval stv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv))!=0)
        logger->Warning()<<"Failed to set SO_SNDTIMEO option to socket: "<<strerror(errno);
}

static void SetSocketDefaultTimeouts(std::shared_ptr<ILogger> &logger, int fd, const IConfig &config)
{
    timeval tv{config.GetRWTimeoutSec(),0};
    SetSocketCustomTimeouts(logger,fd,tv);
}

TCPCommService::TCPCommService(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config):
    logger(_logger),
    sender(_sender),
    config(_config),
    epollFd(epoll_create(_config.GetWorkersCount()*2))
{
    handlerIdCounter=0;
    shutdownPending.store(false);
    events=std::make_unique<epoll_event[]>(config.GetWorkersSpawnCount());
}

CommHandler TCPCommService::GetHandler(const uint64_t id)
{
    const std::lock_guard<std::mutex> guard(manageLock);
    auto h=commHandlers.find(id);
    if(h==commHandlers.end())
        return CommHandler{std::shared_ptr<ICommHelper>(),std::shared_ptr<ICommHelper>(),-1};
    return h->second;
}

uint64_t TCPCommService::ConnectAndCreateHandler(const IPEndpoint &target, const timeval &timeout)
{
    //create socket
    auto fd=socket(target.address.isV6?AF_INET6:AF_INET,SOCK_STREAM,0);
    if(fd<0)
    {
        HandleError(errno,"Failed to create new socket: ");
        return HANDLER_ERROR_OTHER;
    }

    TuneSocketBaseParams(logger,fd,config);
    SetSocketCustomTimeouts(logger,fd,timeout);

    int cr=-1;
    if(target.address.isV6)
    {
        sockaddr_in6 v6sa={};
        target.address.ToSA(&v6sa);
        v6sa.sin6_port=htons(target.port);
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v6sa), sizeof(v6sa));
    }
    else
    {
        sockaddr_in v4sa={};
        target.address.ToSA(&v4sa);
        v4sa.sin_port=htons(target.port);
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v4sa), sizeof(v4sa));
    }

    if(cr<0)
    {
        auto error=errno;
        if(error!=EINPROGRESS)
            logger->Warning()<<"Failed to connect "<<target.address<<" with error: "<<strerror(error);
        else
            logger->Warning()<<"Connection attempt to "<<target.address<<" timed out";
        if(error==ECONNREFUSED||error==EINPROGRESS)
            return HANDLER_ERROR_CONN_REFUSED;
        if(error==ENETUNREACH)
            return HANDLER_ERROR_NET_UNAVAIL;
        if(close(fd)!=0)
        {
            HandleError(error,"Failed to perform proper socket close after connection failure: ");
            return HANDLER_ERROR_OTHER;
        }
    }

    return CreateHandlerFromSocket(fd);
}

uint64_t TCPCommService::CreateHandlerFromSocket(const int fd)
{
    //set some parameters to make this socket compatible with non-blocking io and work with TCPCommHelper
    TuneSocketBaseParams(logger,fd,config);
    SetSocketDefaultTimeouts(logger,fd,config);
    SetSocketNonBlocking(logger,fd);
    //create comm-helper objects and setup this socket for epoll listening
    const std::lock_guard<std::mutex> guard(manageLock);
    uint64_t id=++handlerIdCounter;
    CommHandler handler;
    handler.reader=std::make_shared<TCPCommHelper>(logger,config,fd,true);
    handler.writer=std::make_shared<TCPCommHelper>(logger,config,fd,false);
    handler.fd=fd;
    commHandlers.insert({id,handler});
    epoll_event ev;
    ev.events=EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
    ev.data.u64=id;
    if(epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&ev)<0)
        HandleError(errno,"Failed to register socket for epoll processing: ");
    return id;
}

void TCPCommService::DisposeHandler(const uint64_t id)
{
    const std::lock_guard<std::mutex> guard(manageLock);
    auto h=commHandlers.find(id);
    if(h!=commHandlers.end())
    {
        auto fd=h->second.fd;
        commHandlers.erase(h);
        //close on deregister
        if(epoll_ctl(epollFd,EPOLL_CTL_DEL,fd,nullptr)<0)
            HandleError(errno,"Failed to remove fd from epoll processing: ");
        if(close(fd)!=0)
            HandleError(errno,"Failed to close socket: ");
    }
}

void TCPCommService::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void TCPCommService::Worker()
{
    if(epollFd<0)
    {
        HandleError(errno,"Failed to initialize epoll: ");
        return;
    }

    logger->Info()<<"Listening for epoll events for active connections"<<std::endl;

    while(!shutdownPending.load())
    {
        //wait for events
        auto result=epoll_wait(epollFd,events.get(),config.GetWorkersSpawnCount(),config.GetServiceIntervalMS());
        if(result==0)
            continue;
        else if(result<0)
        {
            int error=errno;
            if(error!=EINTR)
            {
                HandleError(error,"epoll_wait failed with error: ");
                return;
            }
            continue;
        }

        //lock while processing events
        {
            const std::lock_guard<std::mutex> guard(manageLock);
            //loop through events
            for(auto i=0;i<result;++i)
            {
                auto id=events[i].data.u64;
                auto ev=events[i].events;

                //get comm-helper for event
                auto h=commHandlers.find(id);
                if(h==commHandlers.end())
                {
                    //should not happen
                    logger->Warning()<<"Failed to process event from not registered handler id: "<<id;
                    continue;
                }

                //handle events, modify epoll interest-list, send notifications to reader/writer
                auto reader=reinterpret_cast<TCPCommHelper*>(h->second.reader.get());
                auto writer=reinterpret_cast<TCPCommHelper*>(h->second.writer.get());

                //hup or error - should be send both to reader and writer
                if((ev&EPOLLERR)!=0||(ev&EPOLLHUP)!=0)
                {
                    reader->NotifyHUP();
                    writer->NotifyHUP();
                    continue;
                }

                //rd hup or data availability
                if((ev&EPOLLRDHUP)!=0)
                    reader->NotifyHUP();
                else if((ev&EPOLLIN)!=0)
                    reader->NotifyDataAvail();

                if((ev&EPOLLOUT)!=0)
                    writer->NotifyDataAvail();
            }
        }
    }

    //notify all monitored handlers to shutdown
    {
        const std::lock_guard<std::mutex> guard(manageLock);
        logger->Info()<<"Sending shutdown notifocations for active connection handlers: "<<commHandlers.size();
        for(auto &it:commHandlers)
        {
            auto reader=reinterpret_cast<TCPCommHelper*>(it.second.reader.get());
            auto writer=reinterpret_cast<TCPCommHelper*>(it.second.writer.get());
            reader->Cancel();
            writer->Cancel();
        }
    }

    logger->Info()<<"Epoll event-processing worker terminated"<<std::endl;
}

void TCPCommService::OnShutdown()
{
    shutdownPending.store(true);
}
