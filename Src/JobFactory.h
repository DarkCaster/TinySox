#ifndef JOB_FACTORY_H
#define JOB_FACTORY_H

#include "IJob.h"
#include "IJobFactory.h"

class JobFactory final : public IJobFactory
{
    public:
        void DestroyJob(IJob* const target) final;
        std::vector<IJob*> CreateJobsFromResult(const IJobResult &source) final;
};

#endif //JOB_FACTORY_H
