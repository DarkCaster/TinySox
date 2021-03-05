#ifndef ICOMM_HELPER_H
#define ICOMM_HELPER_H

#include "Config.h"
#include "ILogger.h"
#include <memory>

class ICommHelper
{
    protected:
        std::shared_ptr<ILogger> logger;
        const IConfig &config;
        ICommHelper(std::shared_ptr<ILogger> &_logger, const IConfig &_config): logger(_logger), config(_config) {};
    public:
        virtual int Transfer(unsigned char * const target, const int len, const bool allowPartial) = 0;
        virtual void Shutdown() = 0;
        virtual int GetStatus() = 0;
};

#endif //ICOMM_HELPER_H
