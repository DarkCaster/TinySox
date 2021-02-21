#ifndef SIMPLE_JOB_WORKER_FACTORY_H
#define SIMPLE_JOB_WORKER_FACTORY_H

#include "IJobWorkerFactory.h"

class SimpleJobWorkerFactory final : public IJobWorkerFactory
{
    public:
        IJobWorker* CreateWorker(ILogger &logger, IMessageSender &sender) final;
        void DestroyWorker(IJobWorker* const target) final;
};

#endif //SIMPLE_JOB_WORKER_FACTORY_H

