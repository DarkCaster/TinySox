#ifndef IJOB_H
#define IJOB_H

#include "IJobResult.h"
#include "ILogger.h"

class IJob
{
    public:
        virtual IJobResult Execute(ILogger &logger) = 0;
        virtual void Cancel(ILogger &logger) = 0;
};

#endif
