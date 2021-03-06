#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "IMessageSubscriber.h"
#include "IMessageSender.h"
#include "IMessage.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "ILoggerFactory.h"
#include "IJobWorkerFactory.h"
#include "IJobFactory.h"
#include "IJobWorker.h"
#include "IJob.h"
#include "IConfig.h"

#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>

class JobDispatcher final: public IMessageSubscriber, public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        ILoggerFactory &loggerFactory;
        IJobWorkerFactory &workerFactory;
        IJobFactory &jobFactory;
        IMessageSender &sender;

        const IConfig &config;
        std::atomic<bool> shutdownPending;
        std::atomic<int> msgProcCount;

        unsigned long workerID;

        std::vector<std::shared_ptr<IJobWorker>> freeWorkers;
        std::unordered_map<const void*,std::shared_ptr<IJobWorker>> activeWorkers;
        std::vector<std::shared_ptr<IJobWorker>> finishedWorkers;

        std::mutex freeLock;
        std::mutex activeLock;
        std::mutex disposeLock;

        void HandleError(int ec, const std::string& message);
        void HandleError(const std::string &message);
        void OnMessageInternal(const void* const source, const IJobCompleteMessage& message);

        //non interlocked private methods, may need to be locked outside
        bool _SpawnWorkers(int count);
        std::shared_ptr<IJobWorker> _CreateWorkerInstance();
    public:
        JobDispatcher(std::shared_ptr<ILogger> &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &workerFactory, IJobFactory &jobFactory, IMessageSender &sender, const IConfig &config);
        //methods from WorkerBase
        void Worker() final;
        void OnShutdown() final;
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
};

#endif //JOBDISPATCHER_H
