#ifndef TCP_COMM_SERVICE_H
#define TCP_COMM_SERVICE_H

#include "ILogger.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "IMessageSender.h"
#include "ICommHelper.h"
#include "ICommManager.h"
#include "ICommService.h"

#include <sys/epoll.h>
#include <atomic>
#include <mutex>
#include <unordered_map>

class TCPCommService final: public ICommManager, public ICommService, public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config;
        const int epollFd;
        std::mutex manageLock;
        std::atomic<bool> shutdownPending;
        std::unique_ptr<epoll_event[]> events;
        std::unordered_map<int,CommHandler> commHandlers;
    public:
        TCPCommService(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const IConfig &config);
        //from ICommManager
        CommHandler GetHandler(const int fd) final;
        //from ICommService
        int ConnectAndRegisterSocket(const IPEndpoint &target, const timeval &timeout) final;
        void RegisterActiveSocket(const int fd) final;
        void DeregisterSocket(const int fd) final;
    private:
        void HandleError(int ec, const std::string& message);
        //from WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //TCP_COMM_SERVICE_H
