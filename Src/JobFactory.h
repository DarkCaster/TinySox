#ifndef JOB_FACTORY_H
#define JOB_FACTORY_H

#include "IJobFactory.h"

class JobFactory final : public IJobFactory
{
    public:
        void DestroyJob(IJob* const target) final;
        //TODO: create job-instances for various SOCKS protocol commands, phases and features.
};

#endif //JOB_FACTORY_H
