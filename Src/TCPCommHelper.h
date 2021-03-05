#ifndef TCP_COMM_HELPER_H
#define TCP_COMM_HELPER_H

#include "Config.h"
#include "ILogger.h"
#include "ICommHelper.h"

#include <mutex>
#include <condition_variable>
#include <memory>

class TCPCommHelper : public ICommHelper
{
    protected:
        const int fd;
        const bool isReader;
        std::condition_variable notifyTrigger;
        std::mutex notifyLock;
        uint64_t extCnt;
        uint64_t intCnt;
        int status;
        TCPCommHelper(std::shared_ptr<ILogger> &_logger, const IConfig &_config, const int sockFD, const bool isReader);
    public:
        int Transfer(unsigned char * const target, const int len, const bool allowPartial) final;
        void Shutdown() final;
        void NotifyDataAvail();
        void NotifyHUP();
        void Cancel();
};

#endif //TCP_COMM_HELPER_H
