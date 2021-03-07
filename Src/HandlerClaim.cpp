#include "HandlerClaim.h"

HandlerClaim::HandlerClaim(const uint64_t _handlerID):
    handlerID(_handlerID)
{
    counter.store(0);
}

HandlerClaim::HandlerClaim(const HandlerClaim& other):
    handlerID(other.handlerID)
{
    counter.store(other.counter.load());
}

HandlerClaimState HandlerClaim::Claim()
{
    return HandlerClaimState{counter.fetch_add(1)+1,handlerID};
}

HandlerClaimState HandlerClaim::Disclaim()
{
    return HandlerClaimState{counter.fetch_sub(1)-1,handlerID};
}

HandlerClaimState HandlerClaim::GetState()
{
    return HandlerClaimState{counter.load(),handlerID};
}
