#ifndef IJOB_WORKER_FACTORY_H
#define IJOB_WORKER_FACTORY_H

#include "IMessageSender.h"
#include "IJobWorker.h"
#include "ILogger.h"

#include <memory>

class IJobWorkerFactory
{
    public:
        virtual std::shared_ptr<IJobWorker> CreateWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender) = 0;
};

#endif //IJOB_WORKER_FACTORY_H

