#ifndef JOB_WORKER_H
#define JOB_WORKER_H

#include "IMessageSender.h"
#include "IJobWorker.h"
#include "ILogger.h"

class JobWorker final : public IJobWorker
{
    private:
        ILogger &logger;
    public:
        JobWorker(ILogger &logger, IMessageSender &sender);
    protected:
        //IJobWorker
        bool SetJob(IJob *job) final;
        //worker base
        void Worker() final;
        void OnShutdown() final;
};

#endif //JOB_WORKER_H

