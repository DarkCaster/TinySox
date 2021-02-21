#include "SimpleJobWorkerFactory.h"
#include "SimpleJobWorker.h"

IJobWorker* SimpleJobWorkerFactory::CreateWorker(ILogger &logger, IMessageSender &sender)
{
    return new SimpleJobWorker(logger,sender);
}

void SimpleJobWorkerFactory::DestroyWorker(IJobWorker* const target)
{
    if(target==nullptr||dynamic_cast<SimpleJobWorker*>(target)==nullptr)
        return;
    delete dynamic_cast<SimpleJobWorker*>(target);
}
