#include "Job_ClientHandshake.h"
#include "SocketHelpers.h"
#include "IPAddress.h"
#include "ImmutableStorage.h"

#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>

class JobTerminalResult final: public IJobTerminalResult{ public: JobTerminalResult(const State &_state):IJobTerminalResult(_state){} };
class ModeConnectJobResult final: public IModeConnectJobResult{ public: ModeConnectJobResult(const State &_state):IModeConnectJobResult(_state){} };

Job_ClientHandshake::Job_ClientHandshake(const State &_state, const IConfig &_config):
    IJob(J_HANDSHAKE),
    config(_config),
    state(_state.ClaimAllSockets())
{
    cancelled.store(false);
}

static std::unique_ptr<const IJobResult> FailWithDisclaim(const State &state)
{
    return std::make_unique<const JobTerminalResult>(state.DisclaimAllSockets());
}

std::unique_ptr<const IJobResult> SendAuthFailWithDisclaim(const State &state, TCPSocketHelper &clientHelper)
{
    unsigned char buff[2];
    buff[0]=0x01;
    buff[1]=0x01;
    clientHelper.WriteData(buff,2);
    return std::make_unique<const JobTerminalResult>(state.DisclaimAllSockets());
}

std::unique_ptr<const IJobResult> Job_ClientHandshake::Execute(ILogger& logger)
{
    if(state.socketClaims.size()!=1)
    {
        logger.Error()<<"Job_ClientHandshake: invalid configuration";
        return FailWithDisclaim(state);
    }

    //dumb abstraction for reading/writing data via sockets
    TCPSocketHelper clientHelper(logger,config,state.socketClaimStates[0].socketFD,cancelled);
    const int BUFF_LEN = 512; //this should be enough for any response
    unsigned char buff[BUFF_LEN]={};

    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim(state);
    if(buff[0]!=0x05)
    {
        logger.Warning()<<"Client handshake failed, invalid protocol version: "<<static_cast<int>(buff[0]);
        return FailWithDisclaim(state);
    }

    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim(state);
    int nmethods=buff[0];
    if(clientHelper.ReadData(buff,nmethods,false)<nmethods)
        return FailWithDisclaim(state);

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
        return FailWithDisclaim(state);
    }

    //notify client about selected auth method
    buff[0]=0x05;
    buff[1]=static_cast<unsigned char>(selectedAuthMethod);
    if(clientHelper.WriteData(buff,2)!=2)
    {
        logger.Warning()<<"Client handshake failed, clien disconnected (auth method selection)";
        return FailWithDisclaim(state);
    }

    //login/password auth
    if(selectedAuthMethod==0x02)
    {
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim(state);
        if(buff[0]!=0x01)
        {
            logger.Warning()<<"Client handshake failed, invalid auth version: "<<static_cast<int>(buff[0]);
            return SendAuthFailWithDisclaim(state,clientHelper);
        }
        //username
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim(state);
        int ulen=buff[0];
        if(clientHelper.ReadData(buff,ulen,false)<ulen)
            return FailWithDisclaim(state);
        std::string username(reinterpret_cast<char*>(buff),ulen);
        //password
        if(clientHelper.ReadData(buff,1,false)<1)
            return FailWithDisclaim(state);
        int plen=buff[0];
        if(clientHelper.ReadData(buff,plen,false)<plen)
            return FailWithDisclaim(state);
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
            return FailWithDisclaim(state);
    }

    //connection neogotiation

    //version
    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim(state);
    if(buff[0]!=0x05)
    {
        logger.Warning()<<"Client handshake failed, invalid protocol version: "<<static_cast<int>(buff[0]);
        return FailWithDisclaim(state);
    }

    //cmd+reserved
    if(clientHelper.ReadData(buff,2,false)<2)
        return FailWithDisclaim(state);
    int cmd=buff[0];

    //atyp
    if(clientHelper.ReadData(buff,1,false)<1)
        return FailWithDisclaim(state);
    int atyp=buff[0];

    std::vector<IPAddress> destIPs;
    if(atyp==0x01)
    {
        if(clientHelper.ReadData(buff,IPV4_ADDR_LEN,false)<IPV4_ADDR_LEN)
            return FailWithDisclaim(state);
        IPAddress v4addr(buff,IPV4_ADDR_LEN);
        if(!v4addr.isValid)
            logger.Warning()<<"Invalid ipv4 address provided by client!";
        else
            destIPs.push_back(v4addr);
    }
    else if(atyp==0x04)
    {
        if(clientHelper.ReadData(buff,IPV6_ADDR_LEN,false)<IPV6_ADDR_LEN)
            return FailWithDisclaim(state);
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
            return FailWithDisclaim(state);
        int dnameLen=buff[0];
        if(clientHelper.ReadData(buff,dnameLen,false)<dnameLen)
            return FailWithDisclaim(state);
        std::string dname(reinterpret_cast<char*>(buff),dnameLen);

        logger.Info()<<"Resolving domain: "<<dname;
        auto udns_ctx=dns_new(config.GetBaseUDNSContext());
        if(udns_ctx==nullptr)
        {
            logger.Error()<<"Failed to create new UDNS context: "<<strerror(errno);
            return FailWithDisclaim(state);
        }

        if(dns_open(udns_ctx)<0)
        {
            logger.Error()<<"dns_open failed: "<<strerror(errno);
            dns_free(udns_ctx);
            return FailWithDisclaim(state);
        }

        auto ans6=dns_resolve_a6(udns_ctx, dname.c_str(), config.GetUDNSSearchDomainIsSet()?0:DNS_NOSRCH);
        if(dns_status(udns_ctx)==0)
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
        if(dns_status(udns_ctx)==0)
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
        return FailWithDisclaim(state);
    }

    //read port
    if(clientHelper.ReadData(buff,2,false)<2)
        return FailWithDisclaim(state);
    uint16_t nsport;
    std::memcpy(reinterpret_cast<void*>(&nsport),buff,sizeof(uint16_t));
    auto port=ntohs(nsport);

    //check CMD
    unsigned char rep=0x01;
    auto bindEP=ImmutableStorage<IPEndpoint>(IPEndpoint());
    auto finalState=ImmutableStorage<State>(state);

    if(cmd==0x01) //connect
    {
        if(destIPs.empty())
            logger.Warning()<<"No valid ip addresses to connect";

        //try connect to any ip-address from destIPs, create new socket claim
        for(auto &ip:destIPs)
        {
            //create socket
            auto target=socket(ip.isV6?AF_INET6:AF_INET,SOCK_STREAM,0);

            linger tLinger={1,0};
            if (setsockopt(target, SOL_SOCKET, SO_LINGER, &tLinger, sizeof(linger))!=0)
            {
                logger.Warning()<<"Failed to setup SO_LINGER on outgoing connection"<<strerror(errno);
                if(close(target)!=0)
                    logger.Error()<<"Failed to perform proper socket close after failed setup: "<<strerror(errno);
                return FailWithDisclaim(finalState.Get());
            }

            int cr=-1;
            if(ip.isV6)
            {
                sockaddr_in6 v6sa={};
                ip.ToSA(&v6sa);
                v6sa.sin6_port=htons(port);
                cr=connect(target,reinterpret_cast<sockaddr*>(&v6sa), sizeof(v6sa));
            }
            else
            {
                sockaddr_in v4sa={};
                ip.ToSA(&v4sa);
                v4sa.sin_port=htons(port);
                cr=connect(target,reinterpret_cast<sockaddr*>(&v4sa), sizeof(v4sa));
            }

            if(cr<0)
            {
                auto error=errno;
                logger.Warning()<<"Failed to connect "<<ip<<" with error: "<<strerror(error);
                if(error==ECONNREFUSED)
                    rep=0x05;
                if(error==ENETUNREACH)
                    rep=0x03;
                if(close(target)!=0)
                    logger.Error()<<"Failed to perform proper socket close after connection failure: "<<strerror(errno);
                return FailWithDisclaim(finalState.Get());
            }
            else
            {
                rep=0x00;
                logger.Info()<<"Connected to: "<<ip;
                //get BND.ADDR and BND.PORT
                sockaddr sa;
                socklen_t sl=sizeof(sa);
                if(getsockname(target, &sa, &sl)<0)
                {
                    logger.Error()<<"Call to getsockname failed: "<<strerror(errno);
                    if(close(target)!=0)
                        logger.Error()<<"Failed to perform proper socket close after getsockname failure: "<<strerror(errno);
                    return FailWithDisclaim(finalState.Get());
                }
                IPEndpoint ep(&sa);
                if(!ep.address.isValid)
                {
                    logger.Error()<<"Invalid bind address discovered with getsockname";
                    if(close(target)!=0)
                        logger.Error()<<"Failed to perform proper socket close after failure to decode bind ip-address: "<<strerror(errno);
                    return FailWithDisclaim(finalState.Get());
                }
                bindEP.Set(ep);
                //create new socket claim, update state
                finalState.Set(finalState.Get().AddSocketWithClaim(target));
                break;
            }
        }
    }
    else
    {
        logger.Warning()<<"Command is not supported: "<<cmd;
        rep=0x07;
    }

    //create client response
    buff[0]=0x05; //ver
    buff[1]=rep; //rep
    buff[2]=0x00; //rsv
    auto ep=bindEP.Get();
    buff[3]=ep.address.isV6?0x04:0x01;
    auto respLen=ep.ToRawBuff(buff+4)+4;

    //send client response
    if(clientHelper.WriteData(buff,respLen)<respLen)
    {
        logger.Error()<<"Failed to send response to client";
        return FailWithDisclaim(finalState.Get());
    }

    //TODO: support other commands, like BIND or UDP
    if(cmd==0x01 && rep==0x00)
        return std::make_unique<const ModeConnectJobResult>(finalState.Get().DisclaimAllSockets());
    else
        return FailWithDisclaim(finalState.Get());
}

void Job_ClientHandshake::Cancel(ILogger& logger)
{
    logger.Warning()<<"Cancelling client handshake job";
    cancelled.store(true);
}
