#include "Job_TCPTunnel.h"
#include "SocketHelpers.h"

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };

Job_TCPTunnel::Job_TCPTunnel(const State &_state, const IConfig &_config, const bool _isReader):
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
    TCPSocketReader reader(logger,config,state.socketClaimStates[isReader?1:0].socketFD,cancelled);
    TCPSocketWriter writer(logger,config,state.socketClaimStates[isReader?0:1].socketFD,cancelled);

    //main loop
    while(true)
    {
        auto writerState=writer.GetWriteState();
        if(writerState==-2)
            break;
        if(writerState<0)
        {
            logger->Info()<<(isReader?"(reader)":"(writer)")<<"!WS";
            reader.Shutdown();
            break;
        }

        auto readerState=reader.GetReadState();
        if(readerState==-2)
            break;
        if(readerState<0)
        {
            logger->Info()<<(isReader?"(reader)":"(writer)")<<"!RS";
            writer.Shutdown();
            break;
        }

        if(readerState==0)
        {
            logger->Info()<<(isReader?"(reader)":"(writer)")<<"0RS";
            continue;
        }

        //read available data
        auto dr=reader.ReadData(buff.get(),buffSz,true);
        if(dr==-2)
            break;
        if(dr<1)
        {
            writer.Shutdown();
            break;
        }

        //write data, depending on writer state this may take some time
        auto dw=writer.WriteData(buff.get(),dr);
        if(dw==-2)
            break;
        if(dw<dr)
        {
            reader.Shutdown();
            break;
        }
    }

    return TerminalResultDisclaim(state);
}

void Job_TCPTunnel::Cancel(std::shared_ptr<ILogger> logger)
{
    if(!cancelled->exchange(true))
        logger->Warning()<<"Cancelling TCPTunnel job "<<(isReader?"(reader)":"(writer)");
}
