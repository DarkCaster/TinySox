#include "JobFactory.h"
#include "Job_ClientHandshake.h"

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
        return std::vector<IJob*>{new Job_ClientHandshake(State().AddSocket(result.clientSocketFD),config)};
    }

    //TODO: create job instances based on result from previous job
    return std::vector<IJob*>();
}
