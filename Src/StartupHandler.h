#ifndef STARTUPHANDLER_H
#define STARTUPHANDLER_H

#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IMessage.h"

#include <mutex>
#include <unordered_set>

class StartupContinueMessage: public IStartupContinueMessage { public: StartupContinueMessage():IStartupContinueMessage(){} };

class StartupHandler final: public IMessageSubscriber
{
    private:
        std::mutex lock;
        std::unordered_set<const void *> awaitedTargets;
        bool shutdownRequested;
    public:
        StartupHandler();
        void AddTarget(const void * const target);
        void WaitForStartupReady();
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
};

#endif //STARTUPHANDLER_H

