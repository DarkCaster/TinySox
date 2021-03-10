#include "StartupHandler.h"

StartupHandler::StartupHandler()
{
    shutdownRequested=false;
}

void StartupHandler::AddTarget(const void * const target)
{
    std::lock_guard<std::mutex> guard(lock);
    awaitedTargets.insert(target);
}

void StartupHandler::WaitForStartupReady()
{
    while(true)
    {
        std::lock_guard<std::mutex> guard(lock);
        if(shutdownRequested||awaitedTargets.empty())
            return;
    }
}

bool StartupHandler::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_STARTUP_READY || msgType==MSG_SHUTDOWN;
}

void StartupHandler::OnMessage(const void* const source, const IMessage &message)
{
    std::lock_guard<std::mutex> guard(lock);
    if(message.msgType==MSG_SHUTDOWN)
        shutdownRequested=true;
    else
    {
        auto s=awaitedTargets.find(source);
        if(s!=awaitedTargets.end())
            awaitedTargets.erase(s);
    }
}
