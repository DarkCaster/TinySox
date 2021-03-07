#include "JobFactory.h"
#include "Job_ClientHandshake.h"
#include "Job_TCPTunnel.h"

#include <vector>

JobFactory::JobFactory(std::shared_ptr<ILogger> &_logger, const IConfig &_config, ICommManager &_commManager, ICommService &_commService):
    logger(_logger),
    config(_config),
    commManager(_commManager),
    commService(_commService)
{
}

std::vector<std::shared_ptr<IJob>> JobFactory::CreateJobsFromResult(const IJobResult &source)
{
    //no new jobs needs to be created for JR_TERMINAL job result
    if(source.resultType==JR_TERMINAL)
    {
        //dispose stuff stored in state object
        auto result=static_cast<const IJobTerminalResult&>(source);
        //this will close unused sockets
        for(auto &cState:result.state.handlerClaimStates)
            if(cState.counter<1)
                commService.DisposeHandler(cState.handlerID);
        return std::vector<std::shared_ptr<IJob>>();
    }

    //new client connected, next job will do all the initial handshake routines needed for socks protocol
    if(source.resultType==JR_NEW_CLIENT)
    {
        auto result=static_cast<const INewClientJobResult&>(source);
        return std::vector<std::shared_ptr<IJob>>{ std::make_shared<Job_ClientHandshake>(commService,commManager,config,State().AddHandler(result.handlerID)) };
    }

    //handhake and CONNECT socks command complete, next jobs will transfer data across newly created TCP tunnel
    if(source.resultType==JR_CLIENT_MODE_CONNECT)
    {
        auto result=static_cast<const IModeConnectJobResult&>(source);
        std::vector<std::shared_ptr<IJob>> jobs;
        jobs.push_back(std::make_shared<Job_TCPTunnel>(commManager,result.state,config,true));
        jobs.push_back(std::make_shared<Job_TCPTunnel>(commManager,result.state,config,false));
        return jobs;
    }

    logger->Error()<<"Cannot create new job from unsupported result type from the previous job!";
    return std::vector<std::shared_ptr<IJob>>();
}
