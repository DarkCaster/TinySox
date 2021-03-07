#include "HandlerClaim.h"

HandlerClaim::HandlerClaim(const int _socketFD):
    socketFD(_socketFD)
{
    counter.store(0);
}

HandlerClaim::HandlerClaim(const HandlerClaim& other):
    socketFD(other.socketFD)
{
    counter.store(other.counter.load());
}

HandlerClaimState HandlerClaim::Claim()
{
    return HandlerClaimState{counter.fetch_add(1)+1,socketFD};
}

HandlerClaimState HandlerClaim::Disclaim()
{
    return HandlerClaimState{counter.fetch_sub(1)-1,socketFD};
}

HandlerClaimState HandlerClaim::GetState()
{
    return HandlerClaimState{counter.load(),socketFD};
}
