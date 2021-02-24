#ifndef SOCKETCLAIM_H
#define SOCKETCLAIM_H

#include <atomic>

struct SocketClaimState
{
    public:
        const int counter;
        const int socketFD;
};

class SocketClaim
{
    private:
        std::atomic<int> counter;
        const int socketFD;
    public:
        SocketClaim(const int socketFD);
        SocketClaim(const SocketClaim &other);
        SocketClaimState Claim();
        SocketClaimState Disclaim();
        SocketClaimState GetState();
};

#endif //SOCKETCLAIM_H
