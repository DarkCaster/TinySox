#ifndef IJOB_WORKER_FACTORY_H
#define IJOB_WORKER_FACTORY_H

#include "IJobWorker.h"
#include "ILogger.h"

class IJobWorkerFactory
{
    public:
        virtual IJobWorker* CreateWorker(ILogger &logger) = 0;
        virtual void DestroyWorker(IJobWorker* const target) = 0;
};

#endif //IJOB_WORKER_FACTORY_H

