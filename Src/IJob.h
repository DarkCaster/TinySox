#ifndef IJOB_H
#define IJOB_H

#include "IJobResult.h"

class IJob
{
    public:
        virtual IJobResult Execute() = 0;
        virtual void Cancel() = 0;
};

#endif
