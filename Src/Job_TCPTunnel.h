#ifndef JOB_TCP_TUNNEL_H
#define JOB_TCP_TUNNEL_H


#include "IJob.h"
#include "IJobResult.h"
#include "IConfig.h"
#include "ILogger.h"
#include "State.h"
#include "ICommManager.h"

#include <atomic>
#include <memory>

class Job_TCPTunnel final : public IJob
{
    private:
        ICommManager &commManager;
        const IConfig &config;
        const State state;
        const bool isReader;
        std::atomic<bool> cancel;
    public:
        Job_TCPTunnel(ICommManager &commManager, const State &state, const IConfig &config, const bool isReader);
        //from IJob
        std::unique_ptr<const IJobResult> Execute(std::shared_ptr<ILogger> logger) final;
        void Cancel(std::shared_ptr<ILogger> logger) final;
};


#endif //JOB_TCP_TUNNEL_H
