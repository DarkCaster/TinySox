#include "ClientHandshakeJob.h"

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };
class ModeConnectJobResult final: public IModeConnectJobResult{ public: ModeConnectJobResult(const State &_state):IModeConnectJobResult(_state){} };

ClientHandshakeJob::ClientHandshakeJob(const State &_state, const Config &_config):
    config(_config),
    state(_state.ClaimAllSockets())
{
    cancelled.store(false);
}

std::unique_ptr<const IJobResult> ClientHandshakeJob::Execute(ILogger& logger)
{
    if(state.socketClaims.size()!=1)
    {
        logger.Error()<<"ClientHandshakeJob failed: invalid configuration";
        //generate state for transferring to terminal-job
        return std::unique_ptr<const IJobResult>(new JobTerminalResult(state.DisclaimAllSockets()));
    }
    //TODO: handshake, login, check for supported mode
    //TODO: dns resolve, connect
    //TODO: create another socket, and new socket-claim object

    return std::unique_ptr<const IJobResult>(new ModeConnectJobResult(state));
}

void ClientHandshakeJob::Cancel(ILogger& logger)
{
    logger.Warning()<<"Cancelling client handshake job";
    cancelled.store(true);
}
