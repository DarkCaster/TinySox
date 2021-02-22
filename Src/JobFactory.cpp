#include "JobFactory.h"

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
        return std::vector<IJob*>();
    //TODO: create job instances based on result from previous job
    return std::vector<IJob*>();
}
