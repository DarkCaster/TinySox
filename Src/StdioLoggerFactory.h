#ifndef STDIOLOGGERFACTORY_H
#define STDIOLOGGERFACTORY_H

#include "ILogger.h"
#include "ILoggerFactory.h"
#include <mutex>
#include <atomic>

class StdioLoggerFactory final : public ILoggerFactory
{
    private:
        std::atomic<unsigned int> maxNameWD;
        std::mutex stdioLock;
        double creationTime;
    public:
        StdioLoggerFactory();
        //ILoggerFactory
        ILogger* CreateLogger(const std::string &name) final;
        void DestroyLogger(ILogger* const target) final;
};

#endif // STDIOLOGGERFACTORY_H
