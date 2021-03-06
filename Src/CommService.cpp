#include "CommService.h"

#include <cstring>
#include <sys/epoll.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

CommService::CommService(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config):
    logger(_logger),
    sender(_sender),
    config(_config),
    epollFd(epoll_create(_config.GetWorkersCount()*2))
{
    shutdownPending.store(false);
}

CommHandler CommService::GetHandler(const int fd)
{

}

int CommService::ConnectAndRegisterSocket(const IPEndpoint target, const timeval timeout)
{

}

void CommService::RegisterActiveSocket(const int fd)
{

}

void CommService::DeregisterSocket(const int fd)
{

}

void CommService::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void CommService::Worker()
{
    if(epollFd<0)
    {
        HandleError(errno,"Failed to initialize epoll: ");
        return;
    }

    while(!shutdownPending.load())
    {

    }
}

void CommService::OnShutdown()
{
    shutdownPending.store(true);
}
