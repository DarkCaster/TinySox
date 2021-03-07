#ifndef TCP_COMM_SERVICE_H
#define TCP_COMM_SERVICE_H

#include "ILogger.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "IMessageSender.h"
#include "ICommHelper.h"
#include "ICommManager.h"
#include "ICommService.h"

#include <cstdint>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>

class TCPCommService final: public ICommManager, public ICommService, public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config;
        const int epollFd;
        std::mutex manageLock;
        uint64_t handlerIdCounter;
        std::atomic<bool> shutdownPending;
        std::unique_ptr<epoll_event[]> events;
        std::unordered_map<uint64_t,CommHandler> commHandlers;
    public:
        TCPCommService(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const IConfig &config);
        //from ICommManager
        CommHandler GetHandler(const uint64_t id) final;
        //from ICommService
        uint64_t ConnectAndCreateHandler(const IPEndpoint &target, const timeval &timeout) final;
        uint64_t CreateHandlerFromSocket(const int fd) final;
        void DisposeHandler(const uint64_t id) final;
    private:
        void HandleError(int ec, const std::string& message);
        //from WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //TCP_COMM_SERVICE_H
