#ifndef MESSAGEBROKER_H
#define MESSAGEBROKER_H

#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IMessage.h"

#include <thread>
#include <mutex>
#include <map>
#include <set>

class MessageBroker : public IMessageSender
{
    private:
        std::mutex opLock;
        std::set<IMessageSubscriber*> subscribers;
        std::map<std::thread::id,std::set<const void*>*> callers;
    public:
        void AddSubscriber(IMessageSubscriber& subscriber);
        void SendMessage(const void* const source, const IMessage &message) final;
};

#endif // MESSAGEBROKER_H
