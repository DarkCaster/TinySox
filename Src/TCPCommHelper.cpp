#include "TCPCommHelper.h"
#include <cstring>
#include <unistd.h>

#define N_HUP (UINT64_MAX-1)
#define N_CANCEL (UINT64_MAX)

TCPCommHelper::TCPCommHelper(std::shared_ptr<ILogger> &_logger, const IConfig &_config, const int sockFD, const bool _isReader):
    ICommHelper(_logger, _config),
    fd(sockFD),
    isReader(_isReader)
{
    extCnt=1; //first transfer will try to read/write data, and block only after second attempt
    intCnt=0;
    status=0;
}

static void SetSocketFastShutdown(std::shared_ptr<ILogger> &logger, int fd)
{
    linger cLinger={1,0}; //to save time on shutdown
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
        logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno);
}

int TCPCommHelper::Transfer(unsigned char* const target, const int len, const bool allowPartial)
{
    if(status<0)
        return status;

    if(len<1)
        return 0;

    auto dataLeft=len;
    while(true)
    {
        //wait for data availability or cancel is triggered
        std::unique_lock<std::mutex> lock(notifyLock);
        while(extCnt<=intCnt)
            if(notifyTrigger.wait_for(lock,std::chrono::milliseconds(config.GetSocketTimeoutMS()))==std::cv_status::timeout)
                return 0;
        const auto curCnt=extCnt;
        lock.unlock();

        //immediate return if cancel triggered, do not allow to transfer anything next time
        if(curCnt==N_CANCEL)
        {
            SetSocketFastShutdown(logger,fd);
            status=-2;
            return -2;
        }

        //transfer as much data as possible up to requested size
        auto tSz=isReader?read(fd,(target+len-dataLeft),dataLeft):write(fd,(target+len-dataLeft),dataLeft);
        if(tSz>0)
            dataLeft-=static_cast<int>(tSz);

        //if we transferred all the data - return total size transferred
        if(dataLeft<1)
            return len;

        if(curCnt<N_HUP)
            intCnt=curCnt; //update intCnt to curCnt
        else
            //we received HUP notification and we cannot transfer more data (because dataleft > 0)
            status=-1;

        //partial data transfer
        if(allowPartial && tSz>0)
            return len-dataLeft;

        //tSz<=0, so check error
#if EAGAIN == EWOULDBLOCK
        if(errno!=EAGAIN)
            status=-1;
#else
        auto error=errno;
        if(error!=EAGAIN && error!=EWOULDBLOCK)
            status=-1; //error detected, cannot continue
#endif

        if(status<0)
            return status;
    }

    return -1;
}

void TCPCommHelper::Shutdown()
{
    if(status<-1)
        return;
    shutdown(fd,isReader?SHUT_RD:SHUT_WR);
    status=-2;
}

int TCPCommHelper::GetStatus()
{
    {
        std::lock_guard<std::mutex> lock(notifyLock);
        if(extCnt==N_HUP)
            return -1;
        if(extCnt==N_CANCEL)
            return -2;
    }
    return status;
}

void TCPCommHelper::NotifyDataAvail()
{
    {
        std::lock_guard<std::mutex> lock(notifyLock);
        if(extCnt<N_HUP)
            extCnt++;
    }
    notifyTrigger.notify_one();
}

void TCPCommHelper::NotifyHUP()
{
    {
        std::lock_guard<std::mutex> lock(notifyLock);
        if(extCnt<N_HUP)
            extCnt=N_HUP;
    }
    notifyTrigger.notify_one();
}

void TCPCommHelper::Cancel()
{
    {
        std::lock_guard<std::mutex> lock(notifyLock);
        if(extCnt<N_CANCEL)
            extCnt=N_CANCEL;
    }
    notifyTrigger.notify_one();
}

