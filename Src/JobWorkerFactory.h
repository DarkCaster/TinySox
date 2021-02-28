#ifndef JOB_WORKER_FACTORY_H
#define JOB_WORKER_FACTORY_H

#include "IJobWorkerFactory.h"
#include "IJobWorker.h"
#include "ILogger.h"
#include "IMessageSender.h"

#include <memory>

class JobWorkerFactory final : public IJobWorkerFactory
{
    public:
        std::shared_ptr<IJobWorker> CreateWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender) final;
};

#endif //JOB_WORKER_FACTORY_H

