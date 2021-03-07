#ifndef ICOMM_SERVICE_H
#define ICOMM_SERVICE_H

#include "IPEndpoint.h"

#include <cstdint>
#include <sys/time.h>

#define HANDLER_ERROR_OTHER (UINT64_MAX)
#define HANDLER_ERROR_CONN_REFUSED (UINT64_MAX-1)
#define HANDLER_ERROR_NET_UNAVAIL (UINT64_MAX-2)
#define HANDLER_ERROR (UINT64_MAX-100)

class ICommService
{
    public:
        virtual uint64_t ConnectAndCreateHandler(const IPEndpoint &target, const timeval &timeout) = 0;
        virtual uint64_t CreateHandlerFromSocket(const int fd) = 0;
        virtual void DisposeHandler(const uint64_t id) = 0;
};

#endif //ICOMM_SERVICE_H
