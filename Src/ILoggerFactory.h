#ifndef ILOGGER_FACTORY_H
#define ILOGGER_FACTORY_H

#include "ILogger.h"

class ILoggerFactory
{
    public:
        virtual ILogger* CreateLogger(const std::string &name) = 0;
        virtual void DestroyLogger(ILogger* const target) = 0;
};

#endif // ILOGGER_FACTORY_H

