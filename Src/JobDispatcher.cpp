#include "JobDispatcher.h"

#include <string>
#include <cerrno>
#include <cstring>
#include <sstream>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

JobDispatcher::JobDispatcher(ILogger &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &_workerFactory, IJobFactory &_jobFactory, IMessageSender &_sender, const IConfig &_config):
    logger(dispatcherLogger),
    loggerFactory(workerLoggerFactory),
    workerFactory(_workerFactory),
    jobFactory(_jobFactory),
    sender(_sender),
    config(_config)
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
    //ensure that worker is not executing anything
    instance.worker->Shutdown();
    //destroy all dynamically allocated stuff
    workerFactory.DestroyWorker(instance.worker);
    jobFactory.DestroyJob(instance.job);
    loggerFactory.DestroyLogger(instance.logger);
}

JobDispatcher::WorkerInstance JobDispatcher::_CreateWorkerInstance(IJob *job)
{
    if(freeWorkers.size()<1)
    {
        if(!_SpawnWorkers(1))
            HandleError(errno,"Failed to spawn new worker!");
        return WorkerInstance{nullptr,nullptr,nullptr};
    }
    auto result=freeWorkers.front();
    freeWorkers.pop_front();
    result.job=job;
    return result;
}

void JobDispatcher::Worker()
{
    std::deque<WorkerInstance> tmp;
    uint workersLimit=config.GetWorkersCount();
    uint workersSpawnLimit=config.GetWorkersSpawnCount();
    int mgmInerval=config.GetServiceIntervalMS();

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
                spawnCount=spawnCount>workersSpawnLimit?workersSpawnLimit:spawnCount;
                if(!_SpawnWorkers(static_cast<int>(spawnCount)))
                {
                    HandleError(errno,"Worker startup failed (bg management thread): ");
                    return;
                }
                logger.Info()<<spawnCount<<" new workers spawned, total size: "<<freeWorkers.size();
            }
        }

        //sleep for the next round
        std::this_thread::sleep_for(std::chrono::milliseconds(mgmInerval));
    }

    //spin until no workers processing messages
    while(msgProcCount.load()>0){ }

    //any worker that start processing message at this moment should be aware of shutdownPending value, so it will not spawn any new workers

    //stop and dispose all free workers
    {
        const std::lock_guard<std::mutex> lock(freeLock);
        for(auto &instance:freeWorkers)
        {
            instance.worker->SetJob(nullptr);
            instance.worker->RequestShutdown();
        }
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
    while(msgProcCount.load()>0){ }

    logger.Info()<<"Shuting down JobDispatcher worker thread";
}

void JobDispatcher::OnShutdown()
{
    shutdownPending.store(true);
}

bool JobDispatcher::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_JOB_COMPLETE;
}

void JobDispatcher::OnMessage(const void* const source, const IMessage& message)
{
    msgProcCount.fetch_add(1);
    OnMessageInternal(source,static_cast<const IJobCompleteMessage&>(message));
    msgProcCount.fetch_sub(1);
}

void JobDispatcher::OnMessageInternal(const void* const source, const IJobCompleteMessage& message)
{
    //try to find active worker that sent this message and move it to finishedWorkers to be disposed later
    {
        std::lock_guard<std::mutex> activeGuard(activeLock);
        auto search = activeWorkers.find(source);
        if(search!=activeWorkers.end())
        {
            std::lock_guard<std::mutex> disposeGuard(disposeLock);
            finishedWorkers.push_back(search->second);
            activeWorkers.erase(search);
        }
    }

    //stop creating new jobs if we are currently shutting down
    if(shutdownPending.load())
        return;

    //create new jobs based on previous jobResult and assign every job to new worker
    for(auto job:jobFactory.CreateJobsFromResult(message.result))
    {
        //logger.Info()<<"Preparing worker for new job";
        //get free worker or create a new one + all helper stuff
        std::lock_guard<std::mutex> freeGuard(freeLock);
        auto newInstance=_CreateWorkerInstance(job);
        if(newInstance.worker==nullptr)
            return; //error creating new worker
        //move worker instance to the activeWorkers
        std::lock_guard<std::mutex> activeGuard(activeLock);
        activeWorkers.insert({newInstance.worker,newInstance});
        //assign new job and start it's execution (at worker's thread)
        newInstance.worker->SetJob(newInstance.job);
    }
}
