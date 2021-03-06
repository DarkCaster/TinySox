#ifndef ICOMM_MANAGER_H
#define ICOMM_MANAGER_H

#include "ICommHelper.h"

#include <memory>

struct CommHandler
{
    public:
        std::shared_ptr<ICommHelper> reader;
        std::shared_ptr<ICommHelper> writer;
        static bool IsValid(CommHandler &target) { return target.reader.get()!=nullptr && target.writer.get()!=nullptr; }
};

class ICommManager
{
    public:
        virtual CommHandler GetHandler(const int fd) = 0;
};

#endif //ICOMM_FACTORY_H
