#include "JobFactory.h"
#include "ClientHandshakeJob.h"

#include <vector>


JobFactory::JobFactory(const IConfig &_config):
    config(_config)
{
}

void JobFactory::DestroyJob(IJob* const target)
{
    if(target==nullptr)
        return;


    //TODO:

    //detect job type
    //cast to corresponding job-class
    //free memory
    /*dynamic_cast<REAL_JOB_TYPE*>(target)==nullptr)
    delete dynamic_cast<REAL_JOB_TYPE*>(target);*/
}

std::vector<IJob*> JobFactory::CreateJobsFromResult(const IJobResult &source)
{
    //JR_TERMINAL result means that no new jobs needs to be created
    if(source.resultType==JR_TERMINAL)
    {
        //TODO: process State object and perform cleanup of manually created shared objects
        return std::vector<IJob*>();
    }

    if(source.resultType==JR_NEW_CLIENT)
    {
        auto result=static_cast<const INewClientJobResult&>(source);
        std::vector<std::shared_ptr<SocketClaim>> newSocketClaims;
        std::vector<SocketClaimState> newSocketClaimStates;
        newSocketClaims.push_back(std::make_shared<SocketClaim>(SocketClaim(result.clientSocketFD)));
        newSocketClaimStates.push_back(newSocketClaims[0]->GetState());
        //create state object for the new client connection
        State state(newSocketClaims,newSocketClaimStates);
        return std::vector<IJob*>{new ClientHandshakeJob(state,config)};
    }

    //TODO: create job instances based on result from previous job
    return std::vector<IJob*>();
}
