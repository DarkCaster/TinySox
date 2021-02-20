#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "ILogger.h"
#include "IPAddress.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"

#include <atomic>
#include <sys/time.h>

class JobDispatcher final: public IMessageSubscriber
{
    private:
        ILogger &logger;
        IMessageSender &sender;
        const ushort workersLimit;
        std::atomic<bool> shutdownPending;

        void HandleError(int ec, const std::string& message);
        void HandleError(const std::string &message);
    public:
        JobDispatcher(ILogger &logger, IMessageSender &sender, const ushort workersLimit);
        //startup shutdown
        bool Startup();
        bool Shutdown();
        bool RequestShutdown();
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const IMessage &message) final;
};

#endif //JOBDISPATCHER_H
