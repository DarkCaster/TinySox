#include "State.h"

State::State(const std::vector<std::shared_ptr<HandlerClaim>> &_socketClaims, const std::vector<HandlerClaimState> &_socketClaimStates):
    socketClaims(_socketClaims),
    socketClaimStates(_socketClaimStates)
{
}

State State::ClaimAllSockets() const
{
    std::vector<std::shared_ptr<HandlerClaim>> newSocketClaims;
    std::vector<HandlerClaimState> newSocketClaimStates;
    for(auto &claim:socketClaims)
    {
        newSocketClaimStates.push_back(claim->Claim());
        newSocketClaims.push_back(claim);
    }
    return State(newSocketClaims,newSocketClaimStates);
}

State State::DisclaimAllSockets() const
{
    std::vector<std::shared_ptr<HandlerClaim>> newSocketClaims;
    std::vector<HandlerClaimState> newSocketClaimStates;
    for(auto &claim:socketClaims)
    {
        newSocketClaimStates.push_back(claim->Disclaim());
        newSocketClaims.push_back(claim);
    }
    return State(newSocketClaims,newSocketClaimStates);
}

State State::AddSocket(int fd) const
{
    //copy old claim and it's current states
    std::vector<std::shared_ptr<HandlerClaim>> newSocketClaims;
    std::vector<HandlerClaimState> newSocketClaimStates;
    for(auto &oldClaim:socketClaims)
    {
        newSocketClaims.push_back(oldClaim);
        newSocketClaimStates.push_back(oldClaim->GetState());
    }
    //create new claim
    auto newClaim=std::make_shared<HandlerClaim>(HandlerClaim(fd));
    newSocketClaims.push_back(newClaim);
    newSocketClaimStates.push_back(newClaim->GetState());
    return State(newSocketClaims,newSocketClaimStates);
}

State State::AddSocketWithClaim(int fd) const
{
    //copy old claim and it's current states
    std::vector<std::shared_ptr<HandlerClaim>> newSocketClaims;
    std::vector<HandlerClaimState> newSocketClaimStates;
    for(auto &oldClaim:socketClaims)
    {
        newSocketClaims.push_back(oldClaim);
        newSocketClaimStates.push_back(oldClaim->GetState());
    }
    //create new claim
    auto newClaim=std::make_shared<HandlerClaim>(HandlerClaim(fd));
    newSocketClaims.push_back(newClaim);
    newSocketClaimStates.push_back(newClaim->Claim());
    return State(newSocketClaims,newSocketClaimStates);
}
