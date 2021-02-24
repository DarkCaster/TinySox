#include "State.h"

State::State(const std::vector<std::shared_ptr<SocketClaim> >& _socketClaims, const std::vector<SocketClaimState> &_socketClaimStates):
    socketClaims(_socketClaims),
    socketClaimStates(_socketClaimStates)
{
}

std::vector<std::shared_ptr<SocketClaim>> State::CopyClaims() const
{
    std::vector<std::shared_ptr<SocketClaim>> result;
    for(auto &claim:socketClaims)
        result.push_back(claim);
    return result;
}
