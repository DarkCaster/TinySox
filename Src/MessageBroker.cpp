#include "MessageBroker.h"
#include <vector>

void MessageBroker::AddSubscriber(IMessageSubscriber& subscriber)
{
    const std::lock_guard<std::mutex> lock(opLock);
    subscribers.insert(&subscriber);
}

void MessageBroker::SendMessage(const void* const source, const IMessage& message)
{
    std::set<const void*> newSenders; //will not be used directly
    std::set<const void*> *curSenders=nullptr;
    std::vector<IMessageSubscriber*> curSubscribers;

    //create copy of subscribers list, check senders list for current thread for recursion
    {
        const std::lock_guard<std::mutex> lock(opLock);
        const auto itCallers=callers.find(std::this_thread::get_id());
        if(itCallers==callers.end())
        {//no recursion, just create new storage with senders for current thread
            curSenders=&newSenders;
            curSenders->insert(source);
            callers.insert({std::this_thread::get_id(),curSenders});
        }
        else
        {//recursion detected, we need to check has we already called by specific sender in this thread
            curSenders=itCallers->second;
            if(curSenders->find(source)!=curSenders->end())
                return;//we already processed this sender, we must stop there to prevent infinite recursion
            curSenders->insert(source);
        }
        curSubscribers.assign(subscribers.begin(),subscribers.end());
    }

    for(const auto &subscriber: curSubscribers)
        if(subscriber->ReadyForMessage(message.msgType))
            subscriber->OnMessage(source,message);

    //remove current sender for list
    {
        const std::lock_guard<std::mutex> lock(opLock);
        curSenders->erase(source);
        if(curSenders->empty())
            callers.erase(std::this_thread::get_id());
    }
}
