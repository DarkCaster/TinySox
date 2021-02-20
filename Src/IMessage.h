#ifndef IMESSAGE_H
#define IMESSAGE_H

#include <vector>

enum MsgType
{
    MSG_SHUTDOWN,
    MSG_NEW_CLIENT,
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

class INewClientMessage : public IMessage
{
    protected:
        INewClientMessage(int _fd):IMessage(MSG_NEW_CLIENT),fd(_fd){}
    public:
        const int fd;
};

#endif // IMESSAGE_H
