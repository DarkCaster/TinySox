#include "JobWorkerFactory.h"
#include "JobWorker.h"

IJobWorker* SimpleJobWorkerFactory::CreateWorker(ILogger &logger, IMessageSender &sender)
{
    return new JobWorker(logger,sender);
}

void SimpleJobWorkerFactory::DestroyWorker(IJobWorker* const target)
{
    if(target==nullptr||dynamic_cast<JobWorker*>(target)==nullptr)
        return;
    delete dynamic_cast<JobWorker*>(target);
}
