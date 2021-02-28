#include "JobWorkerFactory.h"
#include "JobWorker.h"

std::shared_ptr<IJobWorker> JobWorkerFactory::CreateWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender)
{
    return std::make_shared<JobWorker>(logger,sender);
}
