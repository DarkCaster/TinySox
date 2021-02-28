#ifndef IJOB_H
#define IJOB_H

#include "IJobResult.h"
#include "ILogger.h"

#include <memory>

enum JobType
{
    J_HANDSHAKE,
    J_CONNECT_TUNNEL,
};


class IJob
{
    protected:
        IJob(const JobType &_jobType):jobType(_jobType){};
    public:
        const JobType jobType;
        virtual std::unique_ptr<const IJobResult> Execute(ILogger &logger) = 0;
        virtual void Cancel(ILogger &logger) = 0;
};

#endif
