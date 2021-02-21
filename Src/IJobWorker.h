#ifndef IJOB_WORKER_H
#define IJOB_WORKER_H

#include "IMessageSender.h"
#include "WorkerBase.h"
#include "IJob.h"

class IJobWorker : public WorkerBase
{
    protected:
        IMessageSender &sender;
        IJobWorker(IMessageSender &_sender):sender(_sender){};
    public:
        virtual bool SetJob(IJob *job) = 0;
};

#endif //JOB_WORKER_H
