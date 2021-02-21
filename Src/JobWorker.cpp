#include "JobWorker.h"

JobWorker::JobWorker(ILogger &_logger, IMessageSender &_sender):
    IJobWorker(_sender),
    logger(_logger)
{
}

bool JobWorker::SetJob(IJob* job)
{
    return false;
}

void JobWorker::Worker()
{
    //TODO:
}

void JobWorker::OnShutdown()
{
    //TODO:
}

