#include "ILogger.h"
#include "StdioLoggerFactory.h"
#include "IPAddress.h"
#include "MessageBroker.h"
#include "ShutdownHandler.h"
#include "JobDispatcher.h"
#include "JobWorkerFactory.h"
#include "JobFactory.h"
#include "TCPServerListener.h"
#include "User.h"
#include "Config.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <csignal>
#include <climits>
#include <sys/time.h>
#include <udns.h>

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters:"<<std::endl;
    std::cerr<<"    -p <port-num> TCP port number to listen at."<<std::endl;
    std::cerr<<"  optional parameters:"<<std::endl;
    std::cerr<<"    -l <ip-addr> listen ip-address"<<std::endl;
    std::cerr<<"    -wc <count> maximum workers count awailable for use immediately"<<std::endl;
    std::cerr<<"    -ws <count> workers to spawn per round"<<std::endl;
    std::cerr<<"    -wt <time, ms> workers management interval"<<std::endl;
    std::cerr<<"    -usr <username> for non anonymous login"<<std::endl;
    std::cerr<<"    -pwd <password> for provided username"<<std::endl;
    std::cerr<<"    -dns <ipaddress> use external DNS server"<<std::endl;
    std::cerr<<"    -src <domain> search domain for use with external DNS"<<std::endl;
    std::cerr<<"    -bsz <bytes> size of TCP buffer used for transferring data"<<std::endl;
}

int param_error(const std::string &self, const std::string &message)
{
    std::cerr<<message<<std::endl;
    usage(self);
    return 1;
}

int main (int argc, char *argv[])
{
    //timeout for main thread waiting for external signals
    const timespec sigTs={2,0};

    std::unordered_map<std::string,std::string> args;
    bool isArgValue=false;
    for(auto i=1;i<argc;++i)
    {
        if(isArgValue)
        {
            args[argv[i-1]]=argv[i];
            isArgValue=false;
            continue;
        }
        if(std::string(argv[i]).length()<2||std::string(argv[i]).front()!='-')
        {
            std::cerr<<"Invalid cmdline argument: "<<argv[i]<<std::endl;
            usage(argv[0]);
            return 1;
        }
        isArgValue=true;
    }

    if(args.empty())
        return param_error(argv[0],"Mandatory parameters are missing!");

    Config config;

    //timeouts used by background workers for network operations and some other events
    //increasing this time will slow down reaction to some internal and external events
    //decreasing this time too much will cause high cpu usage
    config.SetSocketTimeoutMS(1000);

    //parse port number
    if(args.find("-p")==args.end())
        return param_error(argv[0],"TCP port number is missing!");
    auto port=std::atoi(args["-p"].c_str());
    if(port<1||port>65535)
        return param_error(argv[0],"TCP port number is invalid!");

    //TODO: support for providing multiple ip-addresses
    //parse listen address
    if(args.find("-l")!=args.end())
    {
        if(!IPAddress(args["-l"]).isValid||IPAddress(args["-l"]).isV6)
            return param_error(argv[0],"listen IP address is invalid!");
        config.AddListenAddr(IPEndpoint(IPAddress(args["-l"]),static_cast<ushort>(port)));
    }
    else
        config.AddListenAddr(IPEndpoint(IPAddress("127.0.0.1"),static_cast<ushort>(port))); //TODO: add any IP addr support 0.0.0.0

    config.SetWorkersCount(50);
    if(args.find("-wc")!=args.end())
    {
        int cnt=std::atoi(args["-wc"].c_str());
        if(cnt<1 || cnt>1000)
            return param_error(argv[0],"workers count value is invalid!");
        config.SetWorkersCount(cnt);
    }

    config.SetWorkersSpawnCount(10);
    if(args.find("-ws")!=args.end())
    {
        int cnt=std::atoi(args["-wc"].c_str());
        if(cnt<1 || cnt>config.GetWorkersCount())
            return param_error(argv[0],"workers swawn count value is invalid!");
        config.SetWorkersSpawnCount(cnt);
    }

    config.SetServiceIntervalMS(config.GetSocketTimeoutMS());
    if(args.find("-wt")!=args.end())
    {
        int cnt=std::atoi(args["-wt"].c_str());
        if(cnt<100 || cnt>10000)
            return param_error(argv[0],"workers management interval is invalid!");
        config.SetServiceIntervalMS(cnt);
    }

    //TODO: support for multiple users
    if(args.find("-usr")==args.end())
        config.AddUser(User{"",""});
    else
        config.AddUser(User{args["-usr"],args.find("-pwd")==args.end()?"":args["-pwd"]});
    config.AddUser(User{args["-usr"],args.find("-pwd")==args.end()?"":args["-pwd"]});

    //ext DNS setup
    if(args.find("-dns")==args.end())
        dns_init(&dns_defctx,0);
    else
    {
        if(!IPAddress(args["-dns"]).isValid)
            return param_error(argv[0],"DNS address is invalid");
        dns_reset(&dns_defctx);
        if(dns_add_serv(&dns_defctx,IPAddress(args["-dns"]).ToString().c_str())!=1)
            return param_error(argv[0],"Failed to set external DNS server");
        if(args.find("-src")!=args.end())
        {
            if(dns_add_srch(&dns_defctx, args["-src"].c_str())<0)
                return param_error(argv[0],"Failed to set search domain for external DNS server");
            config.SetUDNSSearchDomainIsSet(true);
        }
    }
    config.SetBaseUDNSContext(&dns_defctx);

    //tcp buff size
    config.SetTCPBuffSz(16384);
    if(args.find("-bsz")!=args.end())
    {
        auto bsz=std::atoi(args["-bsz"].c_str());
        if(bsz<128||bsz>131072)
            return param_error(argv[0],"TCP buffer size is invalid");
        config.SetTCPBuffSz(bsz);
    }

    StdioLoggerFactory logFactory;
    auto mainLogger=logFactory.CreateLogger("Main");
    auto dispLogger=logFactory.CreateLogger("Dispatcher");
    auto jobFactoryLogger=logFactory.CreateLogger("JobFactory");
    auto listenerLogger=logFactory.CreateLogger("Listener");

    mainLogger->Info()<<"Starting up";

    //configure essential stuff
    MessageBroker messageBroker;
    ShutdownHandler shutdownHandler;
    messageBroker.AddSubscriber(shutdownHandler);

    //create instances for main logic
    JobWorkerFactory jobWorkerFactory;
    JobFactory jobFactory(*jobFactoryLogger,config);
    JobDispatcher jobDispatcher(*dispLogger,logFactory,jobWorkerFactory,jobFactory,messageBroker,config);
    messageBroker.AddSubscriber(jobDispatcher);

    std::vector<TCPServerListener*> serverListeners;
    for(auto addr:config.GetListenAddrs())
        serverListeners.push_back(new TCPServerListener(*listenerLogger,messageBroker,config,addr));

    //create sigset_t struct with signals
    sigset_t sigset;
    sigemptyset(&sigset);
    if(sigaddset(&sigset,SIGHUP)!=0||sigaddset(&sigset,SIGTERM)!=0||sigaddset(&sigset,SIGUSR1)!=0||sigaddset(&sigset,SIGUSR2)!=0||sigaddset(&sigset,SIGPIPE)!=0||pthread_sigmask(SIG_BLOCK,&sigset,nullptr)!=0)
    {
        mainLogger->Error()<<"Failed to setup signal-handling"<<std::endl;
        return 1;
    }

    //start background workers, or perform post-setup init
    jobDispatcher.Startup();
    for(auto &listener:serverListeners)
        listener->Startup();

    //main loop

    while(true)
    {
        auto signal=sigtimedwait(&sigset,nullptr,&sigTs);
        auto error=errno;
        if(signal<0 && error!=EAGAIN && error!=EINTR)
        {
            mainLogger->Error()<<"Error while handling incoming signal: "<<strerror(error)<<std::endl;
            break;
        }
        else if(signal>0 && signal!=SIGUSR2 && signal!=SIGINT) //SIGUSR2 triggered by shutdownhandler to unblock sigtimedwait
        {
            mainLogger->Info()<< "Pending shutdown by receiving signal: "<<signal<<"->"<<strsignal(signal)<<std::endl;
            break;
        }

        if(shutdownHandler.IsShutdownRequested())
        {
            if(shutdownHandler.GetEC()!=0)
                mainLogger->Error() << "One of background worker was failed, shuting down" << std::endl;
            else
                mainLogger->Info() << "Shuting down gracefully by request from background worker" << std::endl;
            break;
        }
    }

    //request shutdown of background workers
    for(auto &listener:serverListeners) //server TCP listeners will be shutdown first
        listener->RequestShutdown();
    jobDispatcher.RequestShutdown();

    //wait for background workers shutdown complete
    for(auto &listener:serverListeners)
        listener->Shutdown();
    for(auto &listener:serverListeners)
        delete listener;
    jobDispatcher.Shutdown();

    return  0;
}
