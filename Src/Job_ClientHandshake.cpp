#include "Job_ClientHandshake.h"
#include "SocketHelpers.h"
#include "IPAddress.h"

#include <string>
#include <cstring>
#include <netinet/in.h>

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };
class ModeConnectJobResult final: public IModeConnectJobResult{ public: ModeConnectJobResult(const State &_state):IModeConnectJobResult(_state){} };

Job_ClientHandshake::Job_ClientHandshake(const State &_state, const IConfig &_config):
    config(_config),
    state(_state.ClaimAllSockets())
{
    cancelled.store(false);
}

std::unique_ptr<const IJobResult> Job_ClientHandshake::FailWithDisclaim()
{
    return std::unique_ptr<const IJobResult>(new JobTerminalResult(state.DisclaimAllSockets()));
}

std::unique_ptr<const IJobResult> SendAuthFailWithDisclaim(const State &state, TCPSocketHelper &clientHelper)
{
    unsigned char buff[2];
    buff[0]=0x01;
    buff[1]=0x01;
    clientHelper.WriteData(buff,2);
    return std::unique_ptr<const IJobResult>(new JobTerminalResult(state.DisclaimAllSockets()));
}

std::unique_ptr<const IJobResult> Job_ClientHandshake::Execute(ILogger& logger)
{
    if(state.socketClaims.size()!=1)
    {
        logger.Error()<<"ClientHandshakeJob: invalid configuration";
        return FailWithDisclaim();
    }

    //dumb abstraction for reading/writing data via sockets
    TCPSocketHelper clientHelper(logger,config,state.socketClaimStates[0].socketFD,cancelled);
    const int BUFF_LEN = 256;
    unsigned char buff[BUFF_LEN];

    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim();
    if(buff[0]!=0x05)
    {
        logger.Warning()<<"Client handshake failed, invalid protocol version: "<<static_cast<int>(buff[0]);
        return FailWithDisclaim();
    }

    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim();
    int nmethods=buff[0];
    if(clientHelper.ReadData(buff,nmethods,false)<nmethods)
        return FailWithDisclaim();

    //method selection: we can only support 0x00 and 0x02
    int selectedAuthMethod=-1;
    bool anonLoginSupported=config.GetUser(std::string(""))!=nullptr;

    for(int m=0;m<nmethods;++m)
        if((anonLoginSupported && buff[m]==0x00)||buff[m]==0x02)
        {
            selectedAuthMethod=buff[m];
            break;
        }

    if(selectedAuthMethod<0)
    {
        logger.Warning()<<"Client handshake failed, no supported auth method was provided by client";
        buff[0]=0x05;
        buff[1]=0xFF;
        clientHelper.WriteData(buff,2);
        clientHelper.ReadData(buff,BUFF_LEN,true); //wait while client close connection, so reading may fail (this is ok)
        return FailWithDisclaim();
    }

    //notify client about selected auth method
    buff[0]=0x05;
    buff[1]=static_cast<unsigned char>(selectedAuthMethod);
    if(clientHelper.WriteData(buff,2)!=2)
    {
        logger.Warning()<<"Client handshake failed, clien disconnected (auth method selection)";
        return FailWithDisclaim();
    }

    //login/password auth
    if(selectedAuthMethod==0x02)
    {
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim();
        if(buff[0]!=0x01)
        {
            logger.Warning()<<"Client handshake failed, invalid auth version: "<<static_cast<int>(buff[0]);
            return SendAuthFailWithDisclaim(state,clientHelper);
        }
        //username
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim();
        int ulen=buff[0];
        if(clientHelper.ReadData(buff,ulen,false)<ulen)
            return FailWithDisclaim();
        std::string username(reinterpret_cast<char*>(buff),ulen);
        //password
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim();
        int plen=buff[0];
        if(clientHelper.ReadData(buff,plen,false)<plen)
            return FailWithDisclaim();
        std::string password(reinterpret_cast<char*>(buff),plen);
        //check authentification
        auto user=config.GetUser(username);
        if(user==nullptr || user->password!=password)
        {
            logger.Warning()<<"Client handshake failed, login failed";
            return SendAuthFailWithDisclaim(state,clientHelper);
        }
        //confirm login
        buff[0]=0x01;
        buff[1]=0x00;
        if(clientHelper.WriteData(buff,2)!=2)
            return FailWithDisclaim();
    }

    //connection neogotiation

    //version
    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim();
    if(buff[0]!=0x05)
    {
        logger.Warning()<<"Client handshake failed, invalid protocol version: "<<static_cast<int>(buff[0]);
        return FailWithDisclaim();
    }

    //cmd+reserved
    if(clientHelper.ReadData(buff,2,false)<2)
        return FailWithDisclaim();
    int cmd=buff[0];

    //atyp
    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim();
    int atyp=buff[0];

    std::vector<IPAddress> destIPs;
    if(atyp==0x01)
    {
        if(clientHelper.ReadData(buff,IPV4_ADDR_LEN,false)<IPV4_ADDR_LEN)
            return FailWithDisclaim();
        IPAddress v4addr(buff,IPV4_ADDR_LEN);
        if(!v4addr.isValid)
            logger.Warning()<<"Invalid ipv4 address provided by client!";
        else
            destIPs.push_back(v4addr);
    }
    else if(atyp==0x04)
    {
        if(clientHelper.ReadData(buff,IPV6_ADDR_LEN,false)<IPV6_ADDR_LEN)
            return FailWithDisclaim();
        IPAddress v6addr(buff,IPV6_ADDR_LEN);
        if(!v6addr.isValid)
            logger.Warning()<<"Invalid ipv6 address provided by client!";
        else
            destIPs.push_back(v6addr);
    }
    else if(atyp==0x03)
    {
        //dns resolve
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim();
        int dnameLen=buff[0];
        if(clientHelper.ReadData(buff,dnameLen,false)<dnameLen)
            return FailWithDisclaim();
        std::string dname(reinterpret_cast<char*>(buff),dnameLen);

        logger.Info()<<"Resolving domain: "<<dname;
        auto udns_ctx=dns_new(config.GetBaseUDNSContext());
        if(udns_ctx==nullptr)
        {
            logger.Error()<<"Failed to create new UDNS context: "<<strerror(errno);
            return FailWithDisclaim();
        }

        if(dns_open(udns_ctx)<0)
        {
            logger.Error()<<"dns_open failed: "<<strerror(errno);
            dns_free(udns_ctx);
            return FailWithDisclaim();
        }

        auto ans6=dns_resolve_a6(udns_ctx, dname.c_str(), config.GetUDNSSearchDomainIsSet()?0:DNS_NOSRCH);
        auto status=dns_status(udns_ctx);
        if(status<0)
            logger.Info()<<"udns IN AAAA query to host "<<dname<<" failed with status code "<<status;
        else
            for(auto addr_idx=0;addr_idx<ans6->dnsa6_nrr;++addr_idx)
            {
                IPAddress v6Addr(&(ans6->dnsa6_addr[addr_idx]),IPV6_ADDR_LEN);
                if(!v6Addr.isValid)
                    logger.Warning()<<"udns IN AAAA query to host "<<dname<<" produced invalid ipv6 address!";
                else
                    destIPs.push_back(v6Addr);
            }
        if(ans6!=NULL)
            free(ans6);

        auto ans4=dns_resolve_a4(udns_ctx, dname.c_str(), config.GetUDNSSearchDomainIsSet()?0:DNS_NOSRCH);
        status=dns_status(udns_ctx);
        if(status<0)
            logger.Info()<<"udns IN A query to host "<<dname<<" failed with status code "<<status;
        else
            for(auto addr_idx=0;addr_idx<ans4->dnsa4_nrr;++addr_idx)
            {
                IPAddress v4Addr(&(ans4->dnsa4_addr[addr_idx]),IPV4_ADDR_LEN);
                if(!v4Addr.isValid)
                    logger.Warning()<<"udns IN A query to host "<<dname<<" produced invalid ipv4 address!";
                else
                    destIPs.push_back(v4Addr);
            }
        if(ans4!=NULL)
            free(ans4);

        dns_close(udns_ctx);
        dns_free(udns_ctx);
    }
    else
    {
        logger.Warning()<<"Client handshake failed, unsupported address type: "<<atyp;
        return FailWithDisclaim();
    }

    //port
    if(clientHelper.ReadData(buff,2,false)<2)
        return FailWithDisclaim();
    uint16_t nsport;
    std::memcpy(reinterpret_cast<void*>(&nsport),buff,sizeof(uint16_t));
    auto port=ntohs(nsport);

    //TODO: try connect to one of ip-address from destIPs, create new socket claim
    for(auto &ip:destIPs)
        logger.Warning()<<ip;

    return std::unique_ptr<const IJobResult>(new ModeConnectJobResult(state));
}

void Job_ClientHandshake::Cancel(ILogger& logger)
{
    logger.Warning()<<"Cancelling client handshake job";
    cancelled.store(true);
}
