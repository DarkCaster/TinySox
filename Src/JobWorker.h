#ifndef JOB_WORKER_H
#define JOB_WORKER_H

#include "IJobWorker.h"
#include "ILogger.h"
#include "IJob.h"
#include "IMessageSender.h"

#include <mutex>
#include <condition_variable>
#include <memory>

class JobWorker final : public IJobWorker
{
    private:
        std::shared_ptr<ILogger> logger;
        std::condition_variable jobTrigger;
        std::mutex jobLock;
        std::shared_ptr<IJob> job;
        bool jobSet;
    public:
        JobWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender);
    protected:
        //IJobWorker
        bool SetJob(std::shared_ptr<IJob> &job) final;
        //worker base
        void Worker() final;
        void OnShutdown() final;
};

#endif //JOB_WORKER_H

