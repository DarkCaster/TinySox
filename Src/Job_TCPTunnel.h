#ifndef JOB_TCP_TUNNEL_H
#define JOB_TCP_TUNNEL_H


#include "IJob.h"
#include "IJobResult.h"
#include "IConfig.h"
#include "ILogger.h"
#include "State.h"

#include <atomic>
#include <memory>

class Job_TCPTunnel final : public IJob
{
    private:
        const IConfig &config;
        const State state;
        const bool isReader;
        std::atomic<bool> cancelled;
    public:
        Job_TCPTunnel(const State &state, const IConfig &config, const bool isReader);
        //from IJob
        std::unique_ptr<const IJobResult> Execute(ILogger &logger) final;
        void Cancel(ILogger &logger) final;
};


#endif //JOB_TCP_TUNNEL_H
