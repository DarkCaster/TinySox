#include "SocketHelpers.h"

#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

//TODO: needs refactor, maybe, read/write data from/to socket to/from streams ?
//TODO: optimize reading and wtiting logic, use reading/writing in async manner to eliminate unneded work while awaiting for data
TCPSocketHelper::TCPSocketHelper(ILogger &_logger, const IConfig &_config, const int _fd, std::atomic<bool> &_cancel):
    logger(_logger),
    config(_config),
    fd(_fd),
    cancel(_cancel)
{
    readAllowed=true;
    writeAllowed=true;
}

int TCPSocketHelper::ReadData(unsigned char * const target, const int len, const bool allowPartial)
{
    if(!readAllowed)
        return -1;

    auto dataLeft=len;
    while(!cancel.load())
    {
        //wait for data
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        auto ct = config.GetSocketTimeoutTV();
        auto crv = select(fd+1, &set, NULL, NULL, &ct);
        if(crv==0) //no data available for reading, retry
            continue;

        if(crv<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                logger.Warning()<<"Reading (select) interrupted by signal";
            else
                logger.Warning()<<"Reading (select) failed with error: "<<strerror(error);
            readAllowed=false;
            return -1;
        }

        //read data
        auto dataRead=read(fd,reinterpret_cast<void*>(target+len-dataLeft),dataLeft);
        if(dataRead==0)
        {
            logger.Info()<<"Socket has been shutdown (read)"<<std::endl;
            readAllowed=false;
            return len-dataLeft; //return how much we read so far
        }
        if(dataRead<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                logger.Warning()<<"Reading interrupted by signal";
            else
                logger.Warning()<<"Reading failed with error: "<<strerror(error);
            readAllowed=false;
            return -1;
        }
        dataLeft-=static_cast<int>(dataRead);
        if(allowPartial)
            return static_cast<int>(dataRead);
        if(dataLeft>0) //we still need to read more data
            continue;
    }

    logger.Warning()<<"Reading cancelled";
    return -1;
}

int TCPSocketHelper::WriteData(const unsigned char* const target, const int len)
{
    if(!writeAllowed)
        return -1;

    while(!cancel.load())
    {
        //wait for data
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        auto ct = config.GetSocketTimeoutTV();
        auto crv = select(fd+1, NULL, &set, NULL, &ct);
        if(crv==0) //socket is not available for writing
            continue;

        if(crv<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                logger.Warning()<<"Writing (select) interrupted by signal";
            else
                logger.Warning()<<"Writing (select) failed with error: "<<strerror(error);
            writeAllowed=false;
            return -1;
        }

        //read data
        auto dataWritten=write(fd,reinterpret_cast<const void*>(target),len);
        if(dataWritten==0)
        {
            logger.Info()<<"Socket has been shutdown (write)"<<std::endl;
            writeAllowed=false;
            return -1;
        }
        if(dataWritten<len)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                logger.Warning()<<"Writing interrupted by signal";
            else
                logger.Warning()<<"Writing failed with error: "<<strerror(error);
            writeAllowed=false;
        }
        return static_cast<int>(dataWritten);
    }

    logger.Warning()<<"Writing cancelled";
    return -1;
}
