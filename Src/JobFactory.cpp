#include "JobFactory.h"
#include "Job_ClientHandshake.h"
#include "Job_TCPTunnel.h"
#include "SocketHelpers.h"

#include <vector>

JobFactory::JobFactory(std::shared_ptr<ILogger>& _logger, const IConfig &_config):
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

    if(target->jobType==J_CONNECT_TUNNEL)
    {
        delete dynamic_cast<Job_TCPTunnel*>(target);
        return;
    }

    logger->Error()<<"Cannot dispose unknown job type!";
}

std::vector<IJob*> JobFactory::CreateJobsFromResult(const IJobResult &source)
{
    //no new jobs needs to be created for JR_TERMINAL job result
    if(source.resultType==JR_TERMINAL)
    {
        //dispose stuff stored in state object
        auto result=static_cast<const IJobTerminalResult&>(source);
        //this will close unused sockets
        SocketClaimsCleaner::CloseUnclaimedSockets(logger,result.state.socketClaimStates);
        return std::vector<IJob*>();
    }

    //new client connected, next job will do all the initial handshake routines needed for socks protocol
    if(source.resultType==JR_NEW_CLIENT)
    {
        auto result=static_cast<const INewClientJobResult&>(source);
        return std::vector<IJob*>{new Job_ClientHandshake(State().AddSocket(result.clientSocketFD),config)};
    }

    //handhake and CONNECT socks command complete, next jobs will transfer data across newly created TCP tunnel
    if(source.resultType==JR_CLIENT_MODE_CONNECT)
    {
        auto result=static_cast<const IModeConnectJobResult&>(source);
        std::vector<IJob*> jobs;
        jobs.push_back(new Job_TCPTunnel(result.state,config,true));
        jobs.push_back(new Job_TCPTunnel(result.state,config,false));
        return jobs;
    }

    logger->Error()<<"Cannot create new job from unsupported result type from the previous job!";
    return std::vector<IJob*>();
}
