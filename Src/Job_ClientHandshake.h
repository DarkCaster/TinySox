#ifndef JOB_CLIENT_HANDSHAKE_H
#define JOB_CLIENT_HANDSHAKE_H

#include "IJob.h"
#include "IJobResult.h"
#include "State.h"
#include "Config.h"
#include "ILogger.h"

#include <atomic>
#include <vector>
#include <memory>

class Job_ClientHandshake final : public IJob
{
    private:
        const IConfig &config;
        const State state;
        std::atomic<bool> cancelled;
        std::unique_ptr<const IJobResult> FailWithDisclaim();
    public:
        Job_ClientHandshake(const State &state, const IConfig &config);
        //from IJob
        std::unique_ptr<const IJobResult> Execute(ILogger &logger) final;
        void Cancel(ILogger &logger) final;
};

#endif //JOB_CLIENT_HANDSHAKE_H
