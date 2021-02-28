#include "JobWorker.h"
#include "IMessage.h"

class JobCompleteMessage final : public IJobCompleteMessage { public: JobCompleteMessage(const IJobResult &_result):IJobCompleteMessage(_result){} };

JobWorker::JobWorker(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender):
    IJobWorker(_sender),
    logger(_logger)
{
    job=std::shared_ptr<IJob>();
    jobSet=false;
}

bool JobWorker::SetJob(std::shared_ptr<IJob> &_job)
{
    {
        std::lock_guard<std::mutex> lock(jobLock);
        if(jobSet)
            return false;
        job=_job;
        jobSet=true;
    }
    jobTrigger.notify_one();
    return true;
}

void JobWorker::Worker()
{
    // Wait until external logic send a job
    std::unique_lock<std::mutex> lock(jobLock);
    while(!jobSet)
        jobTrigger.wait(lock);
    auto curJob=job;
    lock.unlock();
    if(curJob==nullptr)
        return;
    //execute job and provide result
    auto result=curJob->Execute(logger);
    {
        std::lock_guard<std::mutex> guard(jobLock);
        job=nullptr;
    }
    sender.SendMessage(this, JobCompleteMessage(*result));
}

void JobWorker::OnShutdown()
{
    std::lock_guard<std::mutex> guard(jobLock);
    if(jobSet && job!=nullptr)
        job->Cancel(logger);
}
