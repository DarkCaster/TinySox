#include "JobFactory.h"

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
