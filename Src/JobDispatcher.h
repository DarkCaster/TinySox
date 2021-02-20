#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "ILogger.h"
#include "ILoggerFactory.h"
#include "IPAddress.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IJobWorkerFactory.h"

#include <mutex>
#include <atomic>
#include <deque>
#include <sys/time.h>



class JobDispatcher final: public IMessageSubscriber
{
    private:
        ILogger &logger;
        ILoggerFactory &loggerFactory;
        IJobWorkerFactory &workerFactory;
        IMessageSender &sender;
        const ushort workersLimit;
        std::atomic<bool> shutdownPending;

        struct WorkerInstance { IJobWorker* worker; ILogger* logger; };
        unsigned long workerID;
        std::deque<WorkerInstance> workerPool;
        std::mutex managementLock;

        void HandleError(int ec, const std::string& message);
        void HandleError(const std::string &message);

        //non interlocked private methods, may need to be locked outside
        bool _SpawnWorkers(int count);
    public:
        JobDispatcher(ILogger &dispatcherLogger, ILoggerFactory &workerLoggerFactory, IJobWorkerFactory &workerFactory, IMessageSender &sender, const ushort workersLimit);
        //startup shutdown
        bool Startup();
        bool Shutdown();
        bool RequestShutdown();
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const IMessage &message) final;
};

#endif //JOBDISPATCHER_H
