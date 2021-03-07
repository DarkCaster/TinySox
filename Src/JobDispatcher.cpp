#include "JobDispatcher.h"

#include <string>
#include <cerrno>
#include <cstring>
#include <sstream>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

JobDispatcher::JobDispatcher(std::shared_ptr<ILogger> &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &_workerFactory, IJobFactory &_jobFactory, IMessageSender &_sender, const IConfig &_config):
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
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void JobDispatcher::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

bool JobDispatcher::_SpawnWorkers(int count)
{
    for(int i=0; i<count; ++i)
    {
        std::ostringstream swName;
        swName << "Worker#" << workerID++;
        auto workerLogger=loggerFactory.CreateLogger(swName.str());
        auto newWorker=workerFactory.CreateWorker(workerLogger,sender);
        if(!newWorker->Startup())
            return false;
        freeWorkers.push_back(newWorker);
    }
    return true;
}

std::shared_ptr<IJobWorker> JobDispatcher::_CreateWorkerInstance()
{
    if(freeWorkers.size()<1 && !_SpawnWorkers(1))
    {
        HandleError(errno,"Failed to spawn new worker!");
        return std::shared_ptr<IJobWorker>();
    }
    auto result=freeWorkers.front();
    freeWorkers.pop_front();
    return result;
}

void JobDispatcher::Worker()
{
    std::deque<std::shared_ptr<IJobWorker>> tmp;
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
                instance->RequestShutdown();
            for(auto &instance:tmp)
                instance->Shutdown();
            logger->Info()<<tmp.size()<<" workers disposed";
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
                logger->Info()<<spawnCount<<" new workers spawned, total size: "<<freeWorkers.size();
            }
        }

        //sleep for the next round
        std::this_thread::sleep_for(std::chrono::milliseconds(mgmInerval));
    }

    //spin until no workers processing messages
    logger->Info()<<"Shuting down JobDispatcher: awaiting message processing (pre)";
    while(msgProcCount.load()>0){ }

    //any worker that start processing message at this moment should be aware of shutdownPending value, so it will not spawn any new workers

    //stop and dispose all free workers
    {
        const std::lock_guard<std::mutex> guard(freeLock);
        for(auto &instance:freeWorkers)
        {
            std::shared_ptr<IJob> emptyJob;
            instance->SetJob(emptyJob);
            instance->RequestShutdown();
        }
        for(auto &instance:freeWorkers)
            instance->Shutdown();
        freeWorkers.clear();
        logger->Info()<<"Free-workers pool was shutdown";
    }

    //stop and dispose all active workers running jobs
    {
        const std::lock_guard<std::mutex> guard(activeLock);
        for(auto &instance:activeWorkers)
            instance.second->RequestShutdown();
        for(auto &instance:activeWorkers)
            instance.second->Shutdown();
        activeWorkers.clear();
        logger->Info()<<"Active workers was shutdown";
    }

    //dispose all finished workers
    {
        const std::lock_guard<std::mutex> guard(disposeLock);
        for(auto &instance:finishedWorkers)
            instance->RequestShutdown();
        for(auto &instance:finishedWorkers)
            instance->Shutdown();
        finishedWorkers.clear();
        logger->Info()<<"Finished workers was shutdown";
    }

    //spin until no workers processing messages
    logger->Info()<<"Shuting down JobDispatcher: awaiting message processing (post)";
    while(msgProcCount.load()>0){ }

    logger->Info()<<"Shuting down JobDispatcher complete";
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
    //stop creating new jobs and messing with the locks if shuting down
    //shutdown logic at Worker thread will do the job
    if(shutdownPending.load())
        return;

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

    //create new jobs based on previous jobResult and assign every job to new worker
    for(auto &job:jobFactory.CreateJobsFromResult(message.result))
    {
        //logger->Info()<<"Preparing worker for new job";
        //get free worker or create a new one + all helper stuff
        std::lock_guard<std::mutex> freeGuard(freeLock);
        auto newInstance=_CreateWorkerInstance();
        if(newInstance==nullptr)
            return; //error creating new worker
        //move worker instance to the activeWorkers
        std::lock_guard<std::mutex> activeGuard(activeLock);
        activeWorkers.insert({newInstance.get(),newInstance});
        //assign new job and start it's execution (at worker's thread)
        newInstance->SetJob(job);
    }
}
