#ifndef IJOB_FACTORY_H
#define IJOB_FACTORY_H

#include "IJob.h"
#include "IJobResult.h"

#include <vector>
#include <memory>

class IJobFactory
{
    public:
        virtual std::vector<std::shared_ptr<IJob>> CreateJobsFromResult(const IJobResult &source) = 0;
};

#endif //IJOB_FACTORY_H
