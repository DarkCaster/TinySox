#ifndef IJOB_H
#define IJOB_H

#include "IJobResult.h"
#include "ILogger.h"

#include <memory>

class IJob
{
    public:
        virtual std::unique_ptr<const IJobResult> Execute(ILogger &logger) = 0;
        virtual void Cancel(ILogger &logger) = 0;
};

#endif
