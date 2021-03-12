#ifndef ICOMM_MANAGER_H
#define ICOMM_MANAGER_H

#include "ICommHelper.h"

#include <cstdint>
#include <memory>

struct CommHandler
{
    public:
        std::shared_ptr<ICommHelper> reader;
        std::shared_ptr<ICommHelper> writer;
        int fd;
        static bool IsValid(CommHandler &target) { return target.reader.get()!=nullptr && target.writer.get()!=nullptr && target.fd>-1; }
};



class ICommManager
{
    public:
        virtual CommHandler GetHandler(const uint64_t id) = 0;
};

#endif //ICOMM_MANAGER_H
