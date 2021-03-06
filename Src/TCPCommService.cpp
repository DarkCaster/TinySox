#include "TCPCommService.h"
#include "TCPCommHelper.h"
#include "SocketHelpers.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

TCPCommService::TCPCommService(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config):
    logger(_logger),
    sender(_sender),
    config(_config),
    epollFd(epoll_create(_config.GetWorkersCount()*2))
{
    shutdownPending.store(false);
    events=std::make_unique<epoll_event[]>(config.GetWorkersSpawnCount());
}

CommHandler TCPCommService::GetHandler(const int fd)
{
    const std::lock_guard<std::mutex> guard(manageLock);
    auto h=commHandlers.find(fd);
    if(h==commHandlers.end())
        return CommHandler{std::shared_ptr<ICommHelper>(),std::shared_ptr<ICommHelper>()};
    return h->second;
}

int TCPCommService::ConnectAndRegisterSocket(const IPEndpoint &target, const timeval &timeout)
{
    //create socket
    auto fd=socket(target.address.isV6?AF_INET6:AF_INET,SOCK_STREAM,0);
    if(fd<0)
    {
        HandleError(errno,"Failed to create new socket: ");
        return -10;
    }

    SocketHelpers::TuneSocketBaseParams(logger,fd,config);
    SocketHelpers::SetSocketCustomTimeouts(logger,fd,timeout);

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
            return -1;
        if(error==ENETUNREACH)
            return -2;
        if(close(fd)!=0)
        {
            HandleError(error,"Failed to perform proper socket close after connection failure: ");
            return -11;
        }
    }

    RegisterActiveSocket(fd);
    return fd;
}

void TCPCommService::RegisterActiveSocket(const int fd)
{
    //set some parameters to make this socket compatible with non-blocking io and work with TCPCommHelper
    SocketHelpers::TuneSocketBaseParams(logger,fd,config);
    SocketHelpers::SetSocketDefaultTimeouts(logger,fd,config);
    SocketHelpers::SetSocketNonBlocking(logger,fd);
    //create comm-helper objects and setup this socket for epoll listening
    const std::lock_guard<std::mutex> guard(manageLock);
    CommHandler handler;
    handler.reader=std::make_shared<TCPCommHelper>(logger,config,fd,true);
    handler.writer=std::make_shared<TCPCommHelper>(logger,config,fd,false);
    commHandlers.insert({fd,handler});
    epoll_event ev;
    ev.events=EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
    ev.data.fd=fd;
    if(epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&ev)<0)
        HandleError(errno,"Failed to register socket for epoll processing: ");
}

void TCPCommService::DeregisterSocket(const int fd)
{
    const std::lock_guard<std::mutex> guard(manageLock);
    auto h=commHandlers.find(fd);
    if(h!=commHandlers.end())
    {
        //close on deregister
        if(close(fd)!=0)
            logger->Error()<<"Failed to close socket fd "<<fd<<": "<<strerror(errno);
        commHandlers.erase(h);
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
                int fd=events[i].data.fd;
                auto ev=events[i].events;

                //get comm-helper for event
                auto h=commHandlers.find(fd);
                if(h==commHandlers.end())
                {
                    logger->Warning()<<"Failed to process event from not registered fd: "<<fd;
                    if(epoll_ctl(epollFd,EPOLL_CTL_DEL,fd, nullptr)<0)
                        logger->Error()<<"Failed to remove not registered fd from epoll processing: "<<strerror(errno);
                    continue;
                }

                //handle events, modify epoll interest-list, send notifications to reader/writer
                auto reader=reinterpret_cast<TCPCommHelper*>(h->second.reader.get());
                auto writer=reinterpret_cast<TCPCommHelper*>(h->second.writer.get());

                //hup or error - should be send both to reader and writer
                if((ev&EPOLLERR)!=0||(ev&EPOLLHUP)!=0)
                {
                    if(epoll_ctl(epollFd,EPOLL_CTL_DEL,fd, nullptr)<0)
                        logger->Error()<<"Failed to remove fd from epoll processing: "<<strerror(errno);
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
