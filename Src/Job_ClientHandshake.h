#ifndef JOB_CLIENT_HANDSHAKE_H
#define JOB_CLIENT_HANDSHAKE_H

#include "IJob.h"
#include "IJobResult.h"
#include "IConfig.h"
#include "ILogger.h"
#include "State.h"
#include "ICommService.h"
#include "ICommManager.h"

#include <atomic>
#include <memory>

class Job_ClientHandshake final : public IJob
{
    private:
        ICommService &commService;
        ICommManager &commManager;
        const IConfig &config;
        const State state;
        std::shared_ptr<std::atomic<bool>> cancelled;
    public:
        Job_ClientHandshake(ICommService &commService, ICommManager &commManager, const IConfig &config, const State &state);
        //from IJob
        std::unique_ptr<const IJobResult> Execute(std::shared_ptr<ILogger> logger) final;
        void Cancel(std::shared_ptr<ILogger> logger) final;
};

#endif //JOB_CLIENT_HANDSHAKE_H
