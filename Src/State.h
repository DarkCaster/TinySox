#ifndef STATE_H
#define STATE_H

#include "HandlerClaim.h"
#include <vector>
#include <memory>

class State
{
    protected:
        State(const std::vector<std::shared_ptr<HandlerClaim>> &handlerClaims, const std::vector<HandlerClaimState> &handlerClaimStates);
    public:
        State() = default;
        State ClaimAllSockets() const;
        State DisclaimAllSockets() const;
        State AddSocket(int fd) const;
        State AddSocketWithClaim(int fd) const;
        const std::vector<std::shared_ptr<HandlerClaim>> handlerClaims;
        const std::vector<HandlerClaimState> handlerClaimStates;
};

#endif
