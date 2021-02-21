#include "JobWorker.h"

SimpleJobWorker::SimpleJobWorker(ILogger &_logger, IMessageSender &_sender):
    IJobWorker(_sender),
    logger(_logger)
{
}

bool SimpleJobWorker::SetJob(IJob* job)
{
    return false;
}

void SimpleJobWorker::Worker()
{
    //TODO:
}

void SimpleJobWorker::OnShutdown()
{
    //TODO:
}

