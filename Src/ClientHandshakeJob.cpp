#include "ClientHandshakeJob.h"

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };
class ModeConnectJobResult final: public IModeConnectJobResult{ public: ModeConnectJobResult(const State &_state):IModeConnectJobResult(_state){} };

ClientHandshakeJob::ClientHandshakeJob(const State &state, const Config &_config):
    config(_config),
    claims(state.CopyClaims())
{
    cancelled.store(false);
    if(claims.size()==1) //we should have only one socket from client
        claims[0]->Claim(); //claims must be defined at constructor
}

std::unique_ptr<const IJobResult> ClientHandshakeJob::Execute(ILogger& logger)
{
    std::vector<SocketClaimState> finalClaimStates;
    for(auto &claim:claims)
        finalClaimStates.push_back(claim->GetState());
    if(claims.size()!=1)
    {
        logger.Error()<<"ClientHandshakeJob failed: invalid configuration";
        //generate state for transferring to terminal-job
        return std::unique_ptr<const IJobResult>(new JobTerminalResult(State(claims,finalClaimStates)));
    }
    //TODO: handshake, login, check for supported mode
    //TODO: dns resolve, connect
    //TODO: create another socket, and new socket-claim object

    return std::unique_ptr<const IJobResult>(new ModeConnectJobResult(State(claims,finalClaimStates)));
}

void ClientHandshakeJob::Cancel(ILogger& logger)
{
    logger.Warning()<<"Cancelling client handshake job";
    cancelled.store(true);
}
