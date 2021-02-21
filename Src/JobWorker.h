#ifndef SIMPLE_JOB_WORKER_H
#define SIMPLE_JOB_WORKER_H

#include "IMessageSender.h"
#include "IJobWorker.h"
#include "ILogger.h"

class SimpleJobWorker final : public IJobWorker
{
    private:
        ILogger &logger;
    public:
        SimpleJobWorker(ILogger &logger, IMessageSender &sender);
    protected:
        //IJobWorker
        bool SetJob(IJob *job) final;
        //worker base
        void Worker() final;
        void OnShutdown() final;
};

#endif //SIMPLE_JOB_WORKER_H

