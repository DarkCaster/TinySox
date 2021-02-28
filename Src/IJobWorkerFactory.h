#ifndef IJOB_WORKER_FACTORY_H
#define IJOB_WORKER_FACTORY_H

#include "IMessageSender.h"
#include "IJobWorker.h"
#include "ILogger.h"

class IJobWorkerFactory
{
    public:
        virtual IJobWorker* CreateWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender) = 0;
        virtual void DestroyWorker(IJobWorker* const target) = 0;
};

#endif //IJOB_WORKER_FACTORY_H

