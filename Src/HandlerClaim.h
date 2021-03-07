#ifndef HANDLER_CLAIM_H
#define HANDLER_CLAIM_H

#include <atomic>

struct HandlerClaimState
{
    public:
        const int counter;
        const int socketFD;
};

class HandlerClaim
{
    private:
        std::atomic<int> counter;
        const int socketFD;
    public:
        HandlerClaim(const int socketFD);
        HandlerClaim(const HandlerClaim &other);
        HandlerClaimState Claim();
        HandlerClaimState Disclaim();
        HandlerClaimState GetState();
};

#endif //HANDLER_CLAIM_H
