#include "JobDispatcher.h"

#include <string>
#include <cerrno>
#include <cstring>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

JobDispatcher::JobDispatcher(ILogger &_logger, IMessageSender &_sender, const ushort _workersLimit):
    logger(_logger),
    sender(_sender),
    workersLimit(_workersLimit)
{
    shutdownPending.store(false);
}

void JobDispatcher::HandleError(const std::string &message)
{
    logger.Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void JobDispatcher::HandleError(int ec, const std::string &message)
{
    logger.Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

bool JobDispatcher::Startup()
{
    //fillup worker-pool
    return true;
}

bool JobDispatcher::Shutdown()
{
    //notify all active workers to stop
    //wait and dispose all workers
    return true;
}

bool JobDispatcher::RequestShutdown()
{
    //notify all active workers to stop
    return true;
}

bool JobDispatcher::ReadyForMessage(const MsgType msgType)
{
    return false;
}

void JobDispatcher::OnMessage(const IMessage& message)
{

}

//receive new-connection notifications

//receive workers notifications

//dispose workers that have finished it's job and prepare new ones for the worker-pool
