#ifndef JOB_WORKER_FACTORY_H
#define JOB_WORKER_FACTORY_H

#include "IJobWorkerFactory.h"
#include "IJobWorker.h"
#include "ILogger.h"
#include "IMessageSender.h"

class JobWorkerFactory final : public IJobWorkerFactory
{
    public:
        IJobWorker* CreateWorker(ILogger &logger, IMessageSender &sender) final;
        void DestroyWorker(IJobWorker* const target) final;
};

#endif //JOB_WORKER_FACTORY_H

