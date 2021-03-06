#ifndef ICOMM_SERVICE_H
#define ICOMM_SERVICE_H

#include "IPEndpoint.h"
#include <sys/time.h>

class ICommService
{
    public:
        virtual int ConnectAndRegisterSocket(const IPEndpoint &target, const timeval &timeout) = 0;
        virtual void RegisterActiveSocket(const int fd) = 0;
        virtual void DeregisterSocket(const int fd) = 0;
};

#endif //ICOMM_SERVICE_H
