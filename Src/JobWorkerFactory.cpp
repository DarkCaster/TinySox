#include "JobWorkerFactory.h"
#include "JobWorker.h"

IJobWorker* JobWorkerFactory::CreateWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender)
{
    return new JobWorker(logger,sender);
}

void JobWorkerFactory::DestroyWorker(IJobWorker* const target)
{
    if(target==nullptr||dynamic_cast<JobWorker*>(target)==nullptr)
        return;
    delete dynamic_cast<JobWorker*>(target);
}
