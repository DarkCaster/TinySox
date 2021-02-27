#ifndef STATE_H
#define STATE_H

#include "SocketClaim.h"
#include <vector>
#include <memory>

class State
{
    protected:
        State(const std::vector<std::shared_ptr<SocketClaim>> &socketClaims, const std::vector<SocketClaimState> &socketClaimStates);
    public:
        State() = default;
        State ClaimAllSockets() const;
        State DisclaimAllSockets() const;
        State AddSocket(int fd) const;
        State AddSocketWithClaim(int fd) const;
        const std::vector<std::shared_ptr<SocketClaim>> socketClaims;
        const std::vector<SocketClaimState> socketClaimStates;
};

#endif
