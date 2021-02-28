#ifndef IJOB_WORKER_H
#define IJOB_WORKER_H

#include "IMessageSender.h"
#include "WorkerBase.h"
#include "IJob.h"

#include <memory>

class IJobWorker : public WorkerBase
{
    protected:
        IMessageSender &sender;
        IJobWorker(IMessageSender &_sender):sender(_sender){};
    public:
        virtual bool SetJob(std::shared_ptr<IJob> &job) = 0;
};

#endif //IJOB_WORKER_H
