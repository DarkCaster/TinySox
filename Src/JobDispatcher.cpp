#include "JobDispatcher.h"

#include <string>
#include <cerrno>
#include <cstring>
#include <sstream>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

JobDispatcher::JobDispatcher(ILogger &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &_workerFactory, IMessageSender &_sender, const ushort _workersLimit):
    logger(dispatcherLogger),
    loggerFactory(workerLoggerFactory),
    workerFactory(_workerFactory),
    sender(_sender),
    workersLimit(_workersLimit)
{
    shutdownPending.store(false);
    workerID=0;
}

void JobDispatcher::HandleError(const std::string &message)
{
    logger.Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

bool JobDispatcher::_SpawnWorkers(int count)
{
    for(int i=0; i<count; ++i)
    {
        std::ostringstream swName;
        swName << "Worker#" << workerID++;
        auto workerLogger=loggerFactory.CreateLogger(swName.str());
        auto worker=workerFactory.CreateWorker(*workerLogger,sender);
        if(!worker->Startup())
        {
            HandleError(errno,"Worker startup failed: ");
            return false;
        }
        workerPool.push_back(WorkerInstance{worker,workerLogger});
    }
    return true;
}

void JobDispatcher::HandleError(int ec, const std::string &message)
{
    logger.Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

bool JobDispatcher::Startup()
{
    const std::lock_guard<std::mutex> lock(managementLock);
    _SpawnWorkers(workersLimit);
    return true;
}

bool JobDispatcher::Shutdown()
{
    shutdownPending.store(true);
    const std::lock_guard<std::mutex> lock(managementLock);
    //notify all active workers to stop
    for(auto& instance:workerPool)
        instance.worker->RequestShutdown();
    //wait and dispose all workers
    for(auto& instance:workerPool)
    {
        instance.worker->Shutdown();
        workerFactory.DestroyWorker(instance.worker);
        loggerFactory.DestroyLogger(instance.logger);
    }
    workerPool.clear();
    return true;
}

bool JobDispatcher::RequestShutdown()
{
    shutdownPending.store(true);
    const std::lock_guard<std::mutex> lock(managementLock);
    //notify all active workers to stop
    for(auto& instance:workerPool)
        instance.worker->RequestShutdown();
    return true;
}

bool JobDispatcher::ReadyForMessage(const MsgType msgType)
{
    if(msgType==MSG_NEW_CLIENT)
        return true;
    return false;
}

void JobDispatcher::OnMessage(const IMessage& message)
{
    if(message.msgType==MSG_NEW_CLIENT)
    {
        auto fd=static_cast<const INewClientMessage&>(message).fd;
        logger.Info()<<"TODO: Processing new client connection, fd=="<<fd;
        return;
    }
}

//receive new-connection notifications

//receive workers notifications

//dispose workers that have finished it's job and prepare new ones for the worker-pool
