#ifndef ICOMM_MANAGER_H
#define ICOMM_MANAGER_H

#include "ICommHelper.h"
#include "TCPCommHelper.h"

#include <memory>

struct CommHandler
{
    public:
        bool isValid;
        std::shared_ptr<ICommHelper> reader;
        std::shared_ptr<ICommHelper> writer;
};

class ICommManager
{
    public:
        virtual CommHandler GetHandler(const int fd) = 0;
};

#endif //ICOMM_FACTORY_H
