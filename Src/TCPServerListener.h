#ifndef TCP_SERVER_LISTENER_H
#define TCP_SERVER_LISTENER_H

#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IPEndpoint.h"
#include "ICommService.h"

#include <string>
#include <atomic>
#include <sys/time.h>

class TCPServerListener final : public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        ICommService &commService;
        const IConfig &config;
        const IPEndpoint endpoint;
        std::atomic<bool> shutdownPending;

        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
    public:
        TCPServerListener(std::shared_ptr<ILogger> &logger, IMessageSender &sender, ICommService &commService, const IConfig &config, const IPEndpoint &listenAt);
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //TCP_SERVER_LISTENER_H
