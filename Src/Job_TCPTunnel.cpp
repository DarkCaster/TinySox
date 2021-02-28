#include "Job_TCPTunnel.h"
#include "SocketHelpers.h"

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };

Job_TCPTunnel::Job_TCPTunnel(const State &_state, const IConfig &_config, const bool _isReader):
    IJob(J_CONNECT_TUNNEL),
    config(_config),
    state(_state.ClaimAllSockets()),
    isReader(_isReader)
{
    cancelled=std::make_shared<std::atomic<bool>>();
    cancelled->store(false);
}

static std::unique_ptr<const IJobResult> TerminalResultDisclaim(const State &state)
{
    return std::make_unique<const JobTerminalResult>(state.DisclaimAllSockets());
}

std::unique_ptr<const IJobResult> Job_TCPTunnel::Execute(std::shared_ptr<ILogger> logger)
{
    if(state.socketClaims.size()!=2)
    {
        logger->Error()<<"Job_TCPTunnel: invalid configuration";
        return TerminalResultDisclaim(state);
    }

    int buffSz=config.GetTCPBuffSz();
    auto buff=std::make_unique<unsigned char[]>(buffSz);
    TCPSocketHelper reader(logger,config,state.socketClaimStates[isReader?1:0].socketFD,cancelled);
    TCPSocketHelper writer(logger,config,state.socketClaimStates[isReader?0:1].socketFD,cancelled);

    int dr,dw=-1;

    while(!cancelled->load())
    {
        //read available data
        dr=reader.ReadData(buff.get(),buffSz,true);
        if(dr<1) //socket for reading was closed or shutdown, we must stop here
            break;
        dw=writer.WriteData(buff.get(),dr);
        if(dw<dr) //full write failed, we must stop
            break;
    }

    return TerminalResultDisclaim(state);
}

void Job_TCPTunnel::Cancel(std::shared_ptr<ILogger> logger)
{
    logger->Warning()<<"Cancelling TCP tunnel job";
    cancelled->store(true);
}
