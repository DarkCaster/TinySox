#ifndef IJOB_FACTORY_H
#define IJOB_FACTORY_H

#include "IJob.h"
#include "IJobResult.h"

#include <vector>

class IJobFactory
{
    public:
        virtual void DestroyJob(IJob* const target) = 0;
        virtual std::vector<IJob*> CreateJobsFromResult(const IJobResult &source) = 0;
};

#endif //IJOB_FACTORY_H
