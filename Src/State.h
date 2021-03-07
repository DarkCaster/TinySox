#ifndef STATE_H
#define STATE_H

#include "HandlerClaim.h"
#include <cstdint>
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
        State AddHandler(const uint64_t id) const;
        State AddHandlerWithClaim(const uint64_t id) const;
        const std::vector<std::shared_ptr<HandlerClaim>> handlerClaims;
        const std::vector<HandlerClaimState> handlerClaimStates;
};

#endif
