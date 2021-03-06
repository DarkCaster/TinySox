#ifndef JOB_FACTORY_H
#define JOB_FACTORY_H

#include "IJobFactory.h"
#include "ILogger.h"
#include "IConfig.h"
#include "IJob.h"
#include "IJobResult.h"
#include "ICommManager.h"
#include "ICommService.h"

#include <vector>
#include <memory>

class JobFactory final : public IJobFactory
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig &config;
        ICommManager &commManager;
        ICommService &commService;
    public:
        JobFactory(std::shared_ptr<ILogger> &logger, const IConfig &config, ICommManager &commManager, ICommService &commService);
        std::vector<std::shared_ptr<IJob>> CreateJobsFromResult(const IJobResult &source) final;
};

#endif //JOB_FACTORY_H
