#include "TCPServerListener.h"
#include "IJobResult.h"

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>

#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class JobCompleteMessage: public IJobCompleteMessage { public: JobCompleteMessage(const IJobResult &_result):IJobCompleteMessage(_result){} };
class NewClientJobResult: public INewClientJobResult { public: NewClientJobResult(const uint64_t id):INewClientJobResult(id){} };
class StartupReadyMessage: public IStartupReadyMessage { public: StartupReadyMessage():IStartupReadyMessage(){} };

TCPServerListener::TCPServerListener(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, ICommService &_commService, const IConfig &_config, const IPEndpoint &_listenAt):
    logger(_logger),
    sender(_sender),
    commService(_commService),
    config(_config),
    endpoint(_listenAt)
{
    shutdownPending.store(false);
    startupAllowed.store(false);
}

void TCPServerListener::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void TCPServerListener::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void TCPServerListener::Worker()
{
    if(!endpoint.address.isValid)
    {
        HandleError("Listen IP address is invalid");
        return;
    }

    //create listen socket
    auto lSockFd=socket(endpoint.address.isV6?AF_INET6:AF_INET,SOCK_STREAM,0);
    if(lSockFd==-1)
    {
        HandleError(errno,"Failed to create listen socket: ");
        return;
    }

    //tune some some options
    int sockReuseAddrEnabled=1;
    if (setsockopt(lSockFd, SOL_SOCKET, SO_REUSEADDR, &sockReuseAddrEnabled, sizeof(int))!=0)
    {
        HandleError(errno,"Failed to set SO_REUSEADDR option: ");
        return;
    }
#ifdef SO_REUSEPORT
    int sockReusePortEnabled=1;
    if (setsockopt(lSockFd, SOL_SOCKET, SO_REUSEPORT, &sockReusePortEnabled, sizeof(int))!=0)
    {
        HandleError(errno,"Failed to set SO_REUSEPORT option: ");
        return;
    }
#endif

    linger lLinger={1,0};
    if (setsockopt(lSockFd, SOL_SOCKET, SO_LINGER, &lLinger, sizeof(linger))!=0)
    {
        HandleError(errno,"Failed to set SO_LINGER option: ");
        return;
    }

    sockaddr_in ipv4Addr = {};
    sockaddr_in6 ipv6Addr = {};
    sockaddr *target;
    socklen_t len;
    if(endpoint.address.isV6)
    {
        ipv6Addr.sin6_family=AF_INET6;
        ipv6Addr.sin6_port=htons(static_cast<uint16_t>(endpoint.port));
        endpoint.address.ToSA(&ipv6Addr);
        target=reinterpret_cast<sockaddr*>(&ipv6Addr);
        len=sizeof(sockaddr_in6);
    }
    else
    {
        ipv4Addr.sin_family=AF_INET;
        ipv4Addr.sin_port=htons(static_cast<uint16_t>(endpoint.port));
        endpoint.address.ToSA(&ipv4Addr);
        target=reinterpret_cast<sockaddr*>(&ipv4Addr);
        len=sizeof(sockaddr_in);
    }

    if (bind(lSockFd,target,len)!=0)
    {
        HandleError(errno,"Failed to bind listen socket: ");
        return;
    }

    if (listen(lSockFd,1)!=0)
    {
        HandleError(errno,"Failed to setup listen socket: ");
        return;
    }

    logger->Info()<<"Listening for incoming connection"<<std::endl;

    //notify that we are ready for accepting connections, and wait for startup confirmation
    sender.SendMessage(this, StartupReadyMessage());
    while(!startupAllowed.load()) {}

    pollfd lst;
    lst.fd=lSockFd;
    lst.events=POLLIN;

    while (!shutdownPending.load())
    {
        lst.revents=0;
        auto lrv=poll(&lst,1,config.GetServiceIntervalMS());
        if(lrv==0) //no incoming connection detected
            continue;

        if(lrv<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                break;
            HandleError(error,"Error awaiting incoming connection: ");
            return;
        }

        //accept single connection
        //sockaddr_storage cAddr;
        //socklen_t cAddrSz = sizeof(cAddr);
        //auto cSockFd=accept(lSockFd,reinterpret_cast<sockaddr*>(&cAddr),&cAddrSz);
        auto cSockFd=accept(lSockFd,nullptr,nullptr);
        if(cSockFd<1)
        {
            logger->Warning()<<"Failed to accept connection: "<<strerror(errno)<<std::endl;
            continue;
        }

        //pass client socket FD to the external logic
        //logger->Info()<<"Client connected";
        auto handler=commService.CreateHandlerFromSocket(cSockFd);
        if(handler>HANDLER_ERROR)
        {
            HandleError("Error registering handler for incoming connection socket");
            return;
        }
        //logger->Info()<<"Listener: new client handler id: "<<handler;
        sender.SendMessage(this,JobCompleteMessage(NewClientJobResult(handler)));
    }

    if(close(lSockFd)!=0)
    {
        HandleError(errno,"Failed to close listen socket: ");
        return;
    }

    logger->Info()<<"Shuting down DNSReceiver worker thread"<<std::endl;
}

void TCPServerListener::OnShutdown()
{
    shutdownPending.store(true);
}

bool TCPServerListener::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_STARTUP_CONTINUE;
}

void TCPServerListener::OnMessage(const void* const, const IMessage &)
{
    //continue startup after netns and UID/GID was setup
    startupAllowed.store(true);
}
