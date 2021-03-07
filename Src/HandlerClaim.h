#ifndef HANDLER_CLAIM_H
#define HANDLER_CLAIM_H

#include <cstdint>
#include <atomic>

struct HandlerClaimState
{
    public:
        const int counter;
        const uint64_t handlerID;
};

class HandlerClaim
{
    private:
        std::atomic<int> counter;
        const uint64_t handlerID;
    public:
        HandlerClaim(const uint64_t handlerID);
        HandlerClaim(const HandlerClaim &other);
        HandlerClaimState Claim();
        HandlerClaimState Disclaim();
        HandlerClaimState GetState();
};

#endif //HANDLER_CLAIM_H
