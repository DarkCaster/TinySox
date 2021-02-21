#ifndef IJOB_RESULT_H
#define IJOB_RESULT_H

enum JobResultType
{
    JR_CLIENT_HANDSHAKE_FAILED,
    JR_CLIENT_CONNECT_MODE,
    //JR_CLIENT_BIND_MODE, //not supported currently
    //JR_CLIENT_UDP_MODE, //not supported currently
};

class IJobResult
{
    protected:
        IJobResult(const JobResultType _resultType):resultType(_resultType){};
    public:
        const JobResultType resultType;
};

#endif
