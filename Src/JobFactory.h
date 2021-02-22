#ifndef JOB_FACTORY_H
#define JOB_FACTORY_H

#include "IConfig.h"
#include "IJob.h"
#include "IJobFactory.h"

class JobFactory final : public IJobFactory
{
    private:
        const IConfig &config;
    public:
        JobFactory(const IConfig &config);
        void DestroyJob(IJob* const target) final;
        std::vector<IJob*> CreateJobsFromResult(const IJobResult &source) final;
};

#endif //JOB_FACTORY_H
