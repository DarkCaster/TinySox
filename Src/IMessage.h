#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "IJobResult.h"

enum MsgType
{
    MSG_SHUTDOWN,
    MSG_JOB_COMPLETE,
};

class IMessage
{
    protected:
        IMessage(const MsgType _msgType):msgType(_msgType){};
    public:
        const MsgType msgType;
};

class IShutdownMessage : public IMessage
{
    protected:
        IShutdownMessage(int _ec):IMessage(MSG_SHUTDOWN),ec(_ec){}
    public:
        const int ec;
};

class IJobCompleteMessage : public IMessage
{
    protected:
        IJobCompleteMessage(const IJobResult &_result):IMessage(MSG_JOB_COMPLETE),result(_result){}
    public:
        const IJobResult &result;
};

#endif // IMESSAGE_H
