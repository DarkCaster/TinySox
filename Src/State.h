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
        State ClaimAllHandlers() const;
        State DisclaimAllHandlers() const;
        State AddHandler(int id) const;
        State AddHandlerWithClaim(int id) const;
        const std::vector<std::shared_ptr<HandlerClaim>> handlerClaims;
        const std::vector<HandlerClaimState> handlerClaimStates;
};

#endif
