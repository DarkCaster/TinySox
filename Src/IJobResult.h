#ifndef IJOB_RESULT_H
#define IJOB_RESULT_H

enum JobResultType
{
    JR_TERMINAL,
    JR_NEW_CLIENT,
    //JR_CLIENT_HANDSHAKE_FAILED,
    //JR_CLIENT_MODE_CONNECT,
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

class IJobTerminalResult : public IJobResult
{
    protected:
        IJobTerminalResult():
            IJobResult(JR_TERMINAL)
        {};
};

class INewClientJobResult : public IJobResult
{
    protected:
        INewClientJobResult(const int _clientSocketFD):
            IJobResult(JR_NEW_CLIENT),
            clientSocketFD(_clientSocketFD)
        {};
    public:
        const int clientSocketFD;
};

#endif
