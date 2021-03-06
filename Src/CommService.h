#ifndef COMM_SERVICE_H
#define COMM_SERVICE_H

#include "ILogger.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "IMessageSender.h"
#include "ICommHelper.h"
#include "ICommManager.h"
#include "ICommService.h"

#include <atomic>
#include <mutex>

class CommService final: public ICommManager, public ICommService, public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config;
        const int epollFd;
        std::mutex manageLock;
        std::atomic<bool> shutdownPending;
    public:
        CommService(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config);
        //from ICommManager
        CommHandler GetHandler(const int fd) final;
        //from ICommService
        int ConnectAndRegisterSocket(const IPEndpoint target, const timeval timeout) final;
        void RegisterActiveSocket(const int fd) final;
        void DeregisterSocket(const int fd) final;
    private:
        void HandleError(int ec, const std::string& message);
        //from WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //COMM_SERVICE_H
