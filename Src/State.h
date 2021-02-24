#ifndef STATE_H
#define STATE_H

#include "SocketClaim.h"
#include <vector>
#include <memory>

class State
{
    public:
        State(const std::vector<std::shared_ptr<SocketClaim>> &socketClaims, const std::vector<SocketClaimState> &socketClaimStates);
        State ClaimAllSockets() const;
        State DisclaimAllSockets() const;
        const std::vector<std::shared_ptr<SocketClaim>> socketClaims;
        const std::vector<SocketClaimState> socketClaimStates;
};

#endif
