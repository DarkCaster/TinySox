#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "IConfig.h"
#include "IJob.h"
#include "ILogger.h"
#include "ILoggerFactory.h"
#include "IPAddress.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IJobWorkerFactory.h"
#include "IJobFactory.h"
#include "WorkerBase.h"

#include <mutex>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <sys/time.h>


class JobDispatcher final: public IMessageSubscriber, public WorkerBase
{
    private:
        ILogger &logger;
        ILoggerFactory &loggerFactory;
        IJobWorkerFactory &workerFactory;
        IJobFactory &jobFactory;
        IMessageSender &sender;

        const IConfig &config;
        std::atomic<bool> shutdownPending;
        std::atomic<int> msgProcCount;

        struct WorkerInstance { IJobWorker* worker; ILogger* logger; IJob* job; };
        unsigned long workerID;

        std::deque<WorkerInstance> freeWorkers;
        std::unordered_map<const void*,WorkerInstance> activeWorkers;
        std::deque<WorkerInstance> finishedWorkers;

        std::mutex freeLock;
        std::mutex activeLock;
        std::mutex disposeLock;

        void HandleError(int ec, const std::string& message);
        void HandleError(const std::string &message);
        void OnMessageInternal(const void* const source, const IJobCompleteMessage& message);

        //non interlocked private methods, may need to be locked outside
        bool _SpawnWorkers(int count);
        void _DestroyWorkerInstance(WorkerInstance &instance);
        WorkerInstance _CreateWorkerInstance(IJob *job);
    public:
        JobDispatcher(ILogger &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &workerFactory, IJobFactory &jobFactory, IMessageSender &sender, const IConfig &config);
        //methods from WorkerBase
        void Worker() final;
        void OnShutdown() final;
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
};

#endif //JOBDISPATCHER_H
