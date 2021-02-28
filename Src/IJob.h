#ifndef IJOB_H
#define IJOB_H

#include "IJobResult.h"
#include "ILogger.h"

#include <memory>


class IJob
{
    public:
        virtual std::unique_ptr<const IJobResult> Execute(std::shared_ptr<ILogger> logger) = 0;
        virtual void Cancel(std::shared_ptr<ILogger> logger) = 0;
};

#endif
