#ifndef IJOB_FACTORY_H
#define IJOB_FACTORY_H

#include "IJob.h"

class IJobFactory
{
    public:
        virtual void DestroyJob(IJob* const target) = 0;
        //TODO: methods for creating job-tasks for various socks-protocol activities
};

#endif //IJOB_FACTORY_H
