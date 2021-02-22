#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <csignal>
#include <climits>
#include <udns.h>
#include <sys/time.h>

#include "ILogger.h"
#include "StdioLoggerFactory.h"
#include "IPAddress.h"
#include "MessageBroker.h"
#include "ShutdownHandler.h"
#include "JobDispatcher.h"
#include "JobWorkerFactory.h"
#include "JobFactory.h"
#include "TCPServerListener.h"
#include "Config.h"
#include "tuple"

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
    config.SetSocketTimeoutMS(500);

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

    StdioLoggerFactory logFactory;
    auto mainLogger=logFactory.CreateLogger("Main");
    auto dispLogger=logFactory.CreateLogger("Dispatcher");
    auto listenerLogger=logFactory.CreateLogger("Listener");

    mainLogger->Info()<<"Starting up";

    //configure essential stuff
    MessageBroker messageBroker;
    ShutdownHandler shutdownHandler;
    messageBroker.AddSubscriber(shutdownHandler);

    //create instances for main logic
    JobWorkerFactory jobWorkerFactory;
    JobFactory jobFactory(config);
    JobDispatcher jobDispatcher(*dispLogger,logFactory,jobWorkerFactory,jobFactory,messageBroker,config);
    messageBroker.AddSubscriber(jobDispatcher);

    std::vector<TCPServerListener*> serverListeners;
    for(auto addr:config.GetListenAddrs())
        serverListeners.push_back(new TCPServerListener(*listenerLogger,messageBroker,config,addr));

    //create sigset_t struct with signals
    sigset_t sigset;
    sigemptyset(&sigset);
    if(sigaddset(&sigset,SIGHUP)!=0||sigaddset(&sigset,SIGTERM)!=0||sigaddset(&sigset,SIGUSR1)!=0||sigaddset(&sigset,SIGUSR2)!=0||pthread_sigmask(SIG_BLOCK,&sigset,nullptr)!=0)
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

    logFactory.DestroyLogger(listenerLogger);
    logFactory.DestroyLogger(dispLogger);
    logFactory.DestroyLogger(mainLogger);

    return  0;
}
