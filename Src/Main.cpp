#include "ILogger.h"
#include "StdioLoggerFactory.h"
#include "IPAddress.h"
#include "MessageBroker.h"
#include "StartupHandler.h"
#include "ShutdownHandler.h"
#include "JobDispatcher.h"
#include "JobWorkerFactory.h"
#include "JobFactory.h"
#include "TCPServerListener.h"
#include "TCPCommService.h"
#include "User.h"
#include "Config.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <csignal>
#include <climits>
#include <sys/time.h>
#include <udns.h>

#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters:"<<std::endl;
    std::cerr<<"    -p <port-num> TCP port number to listen at"<<std::endl;
    std::cerr<<"  optional parameters:"<<std::endl;
    std::cerr<<"    -l <ip-addr> IP to listen at, default: 127.0.0.1"<<std::endl;
    std::cerr<<"    -usr <username> user for non-anonymous login, default: not set"<<std::endl;
    std::cerr<<"    -pwd <password> password for provided username, default: not set"<<std::endl;
    std::cerr<<"    -dns <ipaddress> IP for custom DNS server, default: not set"<<std::endl;
    std::cerr<<"    -src <domain> search domain for use with non-default DNS, default: not set"<<std::endl;
    std::cerr<<"  experimental and optimization parameters:"<<std::endl;
    std::cerr<<"    -cmax <seconds> max total time for establishing connection, default: 20"<<std::endl;
    std::cerr<<"    -cmin <seconds> min time for establishing connection to single IP, default: 5"<<std::endl;
    std::cerr<<"    -bsz <bytes> size of TCP buffer used for transferring data, default: 65536"<<std::endl;
    std::cerr<<"    -mt <time, ms> management interval used for some internal routines, default: 500"<<std::endl;
    std::cerr<<"    -st <time, ms> socket timeout, lower to get faster reaction to tunnel disconnection events, default: 5000"<<std::endl;
    std::cerr<<"    -cf <seconds> timeout for flushing data when closing sockets, -1 to disable, 0 - close without flushing, default: 30"<<std::endl;
    std::cerr<<"    -wc <count> maximum workers count awailable for use immediately, default: 50"<<std::endl;
    std::cerr<<"    -ws <count> workers to spawn per round, default: 10"<<std::endl;
    std::cerr<<"    -ns <path> open NETNS provided by this path and create outgoing connections inside it, default: none, examples: /var/run/netns/office, /proc/7777/ns/net"<<std::endl;
    std::cerr<<"  unused parameters, or to be implemented:"<<std::endl;
    std::cerr<<"    -dt <seconds> connection data read/write timeout when transferring data, default: 60, makes no sense when using non-blocking sockets"<<std::endl;
    std::cerr<<"    -hc <seconds> connection idle timeout when only half of the tunnel is closed, default: 30, TODO: currently not used"<<std::endl;
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

    config.SetServiceIntervalMS(500);
    if(args.find("-mt")!=args.end())
    {
        int cnt=std::atoi(args["-mt"].c_str());
        if(cnt<100 || cnt>10000)
            return param_error(argv[0],"workers management interval is invalid!");
        config.SetServiceIntervalMS(cnt);
    }

    config.SetSocketTimeoutMS(5000);
    if(args.find("-st")!=args.end())
    {
        int cnt=std::atoi(args["-st"].c_str());
        if(cnt<100 || cnt>60000)
            return param_error(argv[0],"socket timeout is invalid!");
        config.SetSocketTimeoutMS(cnt);
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
    config.SetTCPBuffSz(65536);
    if(args.find("-bsz")!=args.end())
    {
        auto bsz=std::atoi(args["-bsz"].c_str());
        if(bsz<128||bsz>524288)
            return param_error(argv[0],"TCP buffer size is invalid");
        config.SetTCPBuffSz(bsz);
    }

    //linger
    config.SetLingerSec(30);
    if(args.find("-cf")!=args.end())
    {
        auto time=std::atoi(args["-cf"].c_str());
        if(time<-1||time>600)
            return param_error(argv[0],"Flush timeout value is invalid");
        config.SetLingerSec(time);
    }

    //connection timeouts
    config.SetMinCTimeSec(5);
    if(args.find("-cmin")!=args.end())
    {
        auto time=std::atoi(args["-cmin"].c_str());
        if(time<1||time>60)
            return param_error(argv[0],"Minimal connection timeout is invalid");
        config.SetMinCTimeSec(time);
    }

    config.SetMaxCTimeSec(20);
    if(args.find("-cmax")!=args.end())
    {
        auto time=std::atoi(args["-cmax"].c_str());
        if(time<(config.GetMinCTimeSec()/1000)||time>120)
            return param_error(argv[0],"Maximum connection timeout is invalid");
        config.SetMaxCTimeSec(time);
    }

    config.SetRWTimeSec(60);
    if(args.find("-dt")!=args.end())
    {
        auto time=std::atoi(args["-dt"].c_str());
        if(time<1||time>7200)
            return param_error(argv[0],"Data R|W timeout is invalid");
        config.SetRWTimeSec(time);
    }

    config.SetHalfCloseTimeoutSec(30);
    if(args.find("-hc")!=args.end())
    {
        auto time=std::atoi(args["-hc"].c_str());
        if(time<1||time>7200)
            return param_error(argv[0],"Half-closed connection timeout is invalid");
        config.SetHalfCloseTimeoutSec(time);
    }

    //network namespace
    config.SetNetNS(std::string(""));
    if(args.find("-ns")!=args.end())
        config.SetNetNS(args["-ns"]);

    StdioLoggerFactory logFactory;
    auto mainLogger=logFactory.CreateLogger("Main");
    auto dispLogger=logFactory.CreateLogger("Dispatcher");
    auto jobFactoryLogger=logFactory.CreateLogger("JobFactory");
    auto listenerLogger=logFactory.CreateLogger("Listener");
    auto tcpServiceLogger=logFactory.CreateLogger("TCPCommSvc");

    mainLogger->Info()<<"Starting up";

    //configure the most essential stuff
    MessageBroker messageBroker;
    ShutdownHandler shutdownHandler;
    StartupHandler startupHandler;
    messageBroker.AddSubscriber(shutdownHandler);
    messageBroker.AddSubscriber(startupHandler);

    //create instances for main logic
    JobWorkerFactory jobWorkerFactory;
    TCPCommService tcpCommService(tcpServiceLogger,messageBroker,config);
    JobFactory jobFactory(jobFactoryLogger,config,tcpCommService,tcpCommService);
    JobDispatcher jobDispatcher(dispLogger,logFactory,jobWorkerFactory,jobFactory,messageBroker,config);
    messageBroker.AddSubscriber(jobDispatcher);
    std::vector<std::shared_ptr<TCPServerListener>> serverListeners;
    for(auto &addr:config.GetListenAddrs())
        serverListeners.push_back(std::make_shared<TCPServerListener>(listenerLogger,messageBroker,tcpCommService,config,addr));
    for(auto &listener:serverListeners)
    {
        messageBroker.AddSubscriber(*(listener));
        startupHandler.AddTarget(listener.get());
    }

    //create sigset_t struct with signals
    sigset_t sigset;
    sigemptyset(&sigset);
    if(sigaddset(&sigset,SIGHUP)!=0||sigaddset(&sigset,SIGTERM)!=0||sigaddset(&sigset,SIGUSR1)!=0||sigaddset(&sigset,SIGUSR2)!=0||sigaddset(&sigset,SIGPIPE)!=0||pthread_sigmask(SIG_BLOCK,&sigset,nullptr)!=0)
    {
        mainLogger->Error()<<"Failed to setup signal-handling"<<std::endl;
        return 1;
    }

    //1-st stage startup for priveleged operations
    for(auto &listener:serverListeners)
        listener->Startup();
    startupHandler.WaitForStartupReady();

    if(!shutdownHandler.IsShutdownRequested() && !config.GetNetNS().empty())
    {
        mainLogger->Info()<<"Setting-up network namespace";
        auto nFD=open(config.GetNetNS().c_str(),O_RDONLY);
        if(nFD<0)
        {
            mainLogger->Error()<<"Failed to open netns file: "<<strerror(errno);
            messageBroker.SendMessage(nullptr,ShutdownMessage(1));
        }
        if(!shutdownHandler.IsShutdownRequested())
        {
            auto nsResult=setns(nFD, CLONE_NEWNET);
            if(nsResult<0)
            {
                mainLogger->Error()<<"Failed to set netns: "<<strerror(errno);
                messageBroker.SendMessage(nullptr,ShutdownMessage(1));
            }
        }
    }

    //TODO: change UID/GID,

    //2-nd stage startup, all stuff from here will run unpreveleged and inside new netns
    messageBroker.SendMessage(nullptr,StartupContinueMessage());

    //start background workers, or perform post-setup init
    jobDispatcher.Startup();
    tcpCommService.Startup();

    //main loop, awaiting for signal
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
    tcpCommService.RequestShutdown();
    jobDispatcher.RequestShutdown();

    //wait for background workers shutdown complete
    for(auto &listener:serverListeners)
        listener->Shutdown();
    tcpCommService.Shutdown();
    jobDispatcher.Shutdown();

    return  0;
}
