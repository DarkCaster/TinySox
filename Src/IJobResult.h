#ifndef IJOB_RESULT_H
#define IJOB_RESULT_H

#include <unordered_map>
#include "State.h"

enum JobResultType
{
    JR_TERMINAL, //no new jobs will be spawned from that result
    JR_NEW_CLIENT,
    JR_CLIENT_MODE_CONNECT,
    //JR_CLIENT_MODE_BIND, //not supported currently
    //JR_CLIENT_MODE_UDP, //not supported currently
};

class IJobResult
{
    protected:
        IJobResult(const JobResultType _resultType):
            resultType(_resultType){};
    public:
        const JobResultType resultType;
};

class IJobIntermediateResult : public IJobResult
{
    protected:
        IJobIntermediateResult(const JobResultType _resultType, const State &_state):
            IJobResult(_resultType),
            state(_state)
        {};
    public:
        const State state;
};

class IJobTerminalResult : public IJobIntermediateResult
{
    protected:
        IJobTerminalResult(const State &_state):
            IJobIntermediateResult(JR_TERMINAL,_state)
        {};
};

class INewClientJobResult : public IJobResult
{
    protected:
        INewClientJobResult(const int _handlerID):
            IJobResult(JR_NEW_CLIENT),
            handlerID(_handlerID)
        {};
    public:
        const int handlerID;
};

class IModeConnectJobResult : public IJobIntermediateResult
{
    protected:
        IModeConnectJobResult(const State &_state):
            IJobIntermediateResult(JR_CLIENT_MODE_CONNECT,_state)
        {};
};

#endif
