#include "JobFactory.h"
#include "Job_ClientHandshake.h"
#include "SocketHelpers.h"

#include <vector>

JobFactory::JobFactory(ILogger &_logger, const IConfig &_config):
    logger(_logger),
    config(_config)
{
}

void JobFactory::DestroyJob(IJob* const target)
{
    if(target==nullptr)
        return;

    if(target->jobType==J_HANDSHAKE)
    {
        delete dynamic_cast<Job_ClientHandshake*>(target);
        return;
    }

    logger.Error()<<"Cannot dispose unknown job type!";
}

std::vector<IJob*> JobFactory::CreateJobsFromResult(const IJobResult &source)
{

    //no new jobs needs to be created for JR_TERMINAL job result
    if(source.resultType==JR_TERMINAL)
    {
        //dispose stuff stored in state object
        auto result=static_cast<const IJobTerminalResult&>(source);
        SocketClaimsCleaner::CloseUnclaimedSockets(logger,result.state.socketClaimStates);
        return std::vector<IJob*>();
    }

    if(source.resultType==JR_NEW_CLIENT)
    {
        auto result=static_cast<const INewClientJobResult&>(source);
        return std::vector<IJob*>{new Job_ClientHandshake(State().AddSocket(result.clientSocketFD),config)};
    }

    //TODO: create job instances based on result from previous job
    return std::vector<IJob*>();
}
