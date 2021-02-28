#ifndef JOB_FACTORY_H
#define JOB_FACTORY_H

#include "IJobFactory.h"
#include "ILogger.h"
#include "IConfig.h"
#include "IJob.h"
#include "IJobResult.h"

#include <vector>

class JobFactory final : public IJobFactory
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig &config;
    public:
        JobFactory(std::shared_ptr<ILogger> &logger, const IConfig &config);
        void DestroyJob(IJob* const target) final;
        std::vector<IJob*> CreateJobsFromResult(const IJobResult &source) final;
};

#endif //JOB_FACTORY_H
