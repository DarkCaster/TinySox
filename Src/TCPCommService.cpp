#include "TCPCommService.h"

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

}

int TCPCommService::ConnectAndRegisterSocket(const IPEndpoint target, const timeval timeout)
{

}

void TCPCommService::RegisterActiveSocket(const int fd)
{

}

void TCPCommService::DeregisterSocket(const int fd)
{

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
            it.second.reader->Shutdown();
            it.second.writer->Shutdown();
        }
    }

    logger->Info()<<"Epoll event-processing worker terminated"<<std::endl;
}

void TCPCommService::OnShutdown()
{
    shutdownPending.store(true);
}
