#include "State.h"

State::State(const std::vector<std::shared_ptr<SocketClaim> >& _socketClaims, const std::vector<SocketClaimState> &_socketClaimStates):
    socketClaims(_socketClaims),
    socketClaimStates(_socketClaimStates)
{
}

State State::ClaimAllSockets() const
{
    std::vector<std::shared_ptr<SocketClaim>> newSocketClaims;
    std::vector<SocketClaimState> newSocketClaimStates;
    for(auto &claim:socketClaims)
    {
        newSocketClaimStates.push_back(claim->Claim());
        newSocketClaims.push_back(claim);
    }
    return State(newSocketClaims,newSocketClaimStates);
}

State State::DisclaimAllSockets() const
{
    std::vector<std::shared_ptr<SocketClaim>> newSocketClaims;
    std::vector<SocketClaimState> newSocketClaimStates;
    for(auto &claim:socketClaims)
    {
        newSocketClaimStates.push_back(claim->Disclaim());
        newSocketClaims.push_back(claim);
    }
    return State(newSocketClaims,newSocketClaimStates);
}
