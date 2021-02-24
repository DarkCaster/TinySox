#include "SocketClaim.h"

SocketClaim::SocketClaim(const int _socketFD):
    socketFD(_socketFD)
{
    counter.store(0);
}

SocketClaim::SocketClaim(const SocketClaim& other):
    socketFD(other.socketFD)
{
    counter.store(other.counter.load());
}

SocketClaimState SocketClaim::Claim()
{
    return SocketClaimState{counter.fetch_add(1)+1,socketFD};
}

SocketClaimState SocketClaim::Disclaim()
{
    return SocketClaimState{counter.fetch_sub(1)-1,socketFD};
}

SocketClaimState SocketClaim::GetState()
{
    return SocketClaimState{counter.load(),socketFD};
}
