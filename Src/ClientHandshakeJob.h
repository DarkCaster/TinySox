#ifndef CLIENT_HANDSHAKE_JOB_H
#define CLIENT_HANDSHAKE_JOB_H

#include "IJob.h"
#include "IJobResult.h"
#include "State.h"
#include "Config.h"
#include "ILogger.h"

#include <atomic>
#include <vector>
#include <memory>

class ClientHandshakeJob final : public IJob
{
    private:
        const IConfig &config;
        const State state;
        std::atomic<bool> cancelled;
        std::unique_ptr<const IJobResult> FailWithDisclaim();
    public:
        ClientHandshakeJob(const State &state, const IConfig &config);
        //from IJob
        std::unique_ptr<const IJobResult> Execute(ILogger &logger) final;
        void Cancel(ILogger &logger) final;
};

#endif //CLIENT_HANDSHAKE_JOB_H
