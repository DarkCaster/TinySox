#include "JobDispatcher.h"

#include <string>
#include <cerrno>
#include <cstring>
#include <sstream>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

JobDispatcher::JobDispatcher(ILogger &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &_workerFactory, IJobFactory &_jobFactory, IMessageSender &_sender,
                             const int _workersLimit, const int _workersSpawn, const int _mgmInt):
    logger(dispatcherLogger),
    loggerFactory(workerLoggerFactory),
    workerFactory(_workerFactory),
    jobFactory(_jobFactory),
    sender(_sender),
    workersLimit(_workersLimit),
    workersSpawn(_workersSpawn),
    mgmInerval(_mgmInt)
{
    shutdownPending.store(false);
    msgProcCount.store(0);
    workerID=0;
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

bool JobDispatcher::_SpawnWorkers(int count)
{
    for(int i=0; i<count; ++i)
    {
        std::ostringstream swName;
        swName << "Worker#" << workerID++;
        auto workerLogger=loggerFactory.CreateLogger(swName.str());
        auto newWorker=workerFactory.CreateWorker(*workerLogger,sender);
        if(!newWorker->Startup())
            return false;
        freeWorkers.push_back(WorkerInstance{newWorker,workerLogger,nullptr});
    }
    return true;
}

void JobDispatcher::_DestroyWorkerInstance(JobDispatcher::WorkerInstance &instance)
{
    //ensure we are fully shutdown
    instance.worker->Shutdown();
    //destroy all dynamic instances
    jobFactory.DestroyJob(instance.job);
    loggerFactory.DestroyLogger(instance.logger);
    workerFactory.DestroyWorker(instance.worker);
}

void JobDispatcher::Worker()
{
    std::deque<WorkerInstance> tmp;

    //loop untill shutdown
    while(!shutdownPending.load())
    {
        //dispose workers that finished it's execution
        {
            const std::lock_guard<std::mutex> lock(disposeLock);
            for(auto &instance:finishedWorkers)
                tmp.push_back(instance);
            finishedWorkers.clear();
        }
        if(tmp.size()>0)
        {
            for(auto &instance:tmp)
                instance.worker->RequestShutdown();
            for(auto &instance:tmp)
                _DestroyWorkerInstance(instance);
            logger.Info()<<tmp.size()<<" workers disposed";
            tmp.clear();
        }

        //populate thread pool with new workers if needed
        {
            const std::lock_guard<std::mutex> lock(freeLock);
            if(freeWorkers.size()<workersLimit)
            {
                auto spawnCount=workersLimit-freeWorkers.size();
                if(!_SpawnWorkers(static_cast<int>(spawnCount>workersSpawn?workersSpawn:spawnCount)))
                {
                    HandleError(errno,"Worker startup failed (bg management thread): ");
                    return;
                }
                logger.Info()<<tmp.size()<<" new workers spawned";
            }
        }

        //sleep for the next round
        std::this_thread::sleep_for(std::chrono::milliseconds(mgmInerval));
    }

    //spin until no workers processing messages
    while(msgProcCount.load()<1){ }

    //any worker that start processing message at this moment should be aware of shutdownPending value, so it will not spawn any new workers

    //stop and dispose all free workers
    {
        const std::lock_guard<std::mutex> lock(freeLock);
        for(auto &instance:freeWorkers)
            instance.worker->RequestShutdown();
        for(auto &instance:freeWorkers)
            _DestroyWorkerInstance(instance);
        freeWorkers.clear();
        logger.Info()<<"Free-workers pool was shutdown";
    }

    //stop and dispose all active workers running jobs
    {
        const std::lock_guard<std::mutex> lock(activeLock);
        for(auto &instance:activeWorkers)
            instance.second.worker->RequestShutdown();
        for(auto &instance:activeWorkers)
            _DestroyWorkerInstance(instance.second);
        activeWorkers.clear();
        logger.Info()<<"Active workers was shutdown";
    }

    //dispose all finished workers
    {
        const std::lock_guard<std::mutex> lock(disposeLock);
        for(auto &instance:finishedWorkers)
            instance.worker->RequestShutdown();
        for(auto &instance:finishedWorkers)
            _DestroyWorkerInstance(instance);
        finishedWorkers.clear();
        logger.Info()<<"Finished workers was shutdown";
    }

    //spin until no workers processing messages
    while(msgProcCount.load()<1){ }

    logger.Info()<<"JobDispatcher worker was shutdown";
}

void JobDispatcher::OnShutdown()
{
    shutdownPending.store(true);
}

bool JobDispatcher::ReadyForMessage(const MsgType msgType)
{
    if(msgType==MSG_JOB_COMPLETE || msgType==MSG_NEW_CLIENT)
        return true;
    return false;
}

void JobDispatcher::OnMessage(const IMessage& message)
{
    msgProcCount.fetch_add(1);
    OnMessageInternal(message);
    msgProcCount.fetch_sub(1);
}

void JobDispatcher::OnMessageInternal(const IMessage& message)
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
