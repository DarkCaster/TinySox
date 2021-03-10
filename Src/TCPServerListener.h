#ifndef TCP_SERVER_LISTENER_H
#define TCP_SERVER_LISTENER_H

#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IPEndpoint.h"
#include "ICommService.h"

#include <string>
#include <atomic>
#include <sys/time.h>

class TCPServerListener final : public WorkerBase, public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        ICommService &commService;
        const IConfig &config;
        const IPEndpoint endpoint;
        std::atomic<bool> shutdownPending;
        std::atomic<bool> startupAllowed;

        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
    public:
        TCPServerListener(std::shared_ptr<ILogger> &logger, IMessageSender &sender, ICommService &commService, const IConfig &config, const IPEndpoint &listenAt);
        //IMessageSubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //TCP_SERVER_LISTENER_H
