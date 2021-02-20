#include "SimpleJobWorkerFactory.h"
#include "SimpleJobWorker.h"

IJobWorker* SimpleJobWorkerFactory::CreateWorker(ILogger& logger)
{
    return new SimpleJobWorker(logger);
}

void SimpleJobWorkerFactory::DestroyWorker(IJobWorker* const target)
{
    if(target==nullptr||dynamic_cast<SimpleJobWorker*>(target)==nullptr)
        return;
    delete dynamic_cast<SimpleJobWorker*>(target);
}
