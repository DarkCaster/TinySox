#include "Job_TCPTunnel.h"

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };

Job_TCPTunnel::Job_TCPTunnel(ICommManager &_commManager, const State &_state, const IConfig &_config, const bool _isReader):
    commManager(_commManager),
    config(_config),
    state(_state.ClaimAllHandlers()),
    isReader(_isReader)
{
    cancel.store(false);
}

static std::unique_ptr<const IJobResult> TerminalResultDisclaim(const State &state)
{
    return std::make_unique<const JobTerminalResult>(state.DisclaimAllHandlers());
}

std::unique_ptr<const IJobResult> Job_TCPTunnel::Execute(std::shared_ptr<ILogger> logger)
{
    if(state.handlerClaims.size()!=2)
    {
        logger->Error()<<"Job_TCPTunnel: invalid configuration";
        return TerminalResultDisclaim(state);
    }

    int buffSz=config.GetTCPBuffSz();
    auto buff=std::make_unique<unsigned char[]>(buffSz);
    auto rHandler=commManager.GetHandler(state.handlerClaimStates[isReader?1:0].handlerID);
    auto wHandler=commManager.GetHandler(state.handlerClaimStates[isReader?0:1].handlerID);
    if(!CommHandler::IsValid(rHandler)||!CommHandler::IsValid(wHandler))
    {
        logger->Error()<<"Job_TCPTunnel: failed to get valid CommHandler(s)";
        return TerminalResultDisclaim(state);
    }

    //logger->Info()<<(isReader?"(reader)":"(writer)")<<"Started tunnel rID: "<<state.handlerClaimStates[isReader?1:0].handlerID<<" wID: "<<state.handlerClaimStates[isReader?0:1].handlerID;

    //main loop
    while(!cancel.load())
    {
        //get writer-end current status
        auto status=wHandler.writer->GetStatus();
        if(status<0)
            break;

        //try to read something from reader-end
        auto dr=rHandler.reader->Transfer(buff.get(),buffSz,true);
        if(dr<0)
            break;

        if(dr==0)
            continue;

        //try to write data we read to writer-end
        status=0;
        while(status==0)
        {
            status=wHandler.writer->Transfer(buff.get(),dr,false);
            //also track reader status
            if(status==0)
                status=rHandler.reader->GetStatus();
        }

        //write failed, or reader is gone
        if(status<0)
            break;
    }

    rHandler.reader->Shutdown();
    wHandler.writer->Shutdown();

    //logger->Info()<<(isReader?"(reader)":"(writer)")<<"Closing tunnel rID: "<<state.handlerClaimStates[isReader?1:0].handlerID<<" wID: "<<state.handlerClaimStates[isReader?0:1].handlerID;
    return TerminalResultDisclaim(state);
}

void Job_TCPTunnel::Cancel(std::shared_ptr<ILogger> logger)
{
    if(!cancel.exchange(true))
        logger->Warning()<<"Cancelling TCPTunnel job "<<(isReader?"(reader)":"(writer)");
}
