#include "State.h"

State::State(const std::vector<std::shared_ptr<HandlerClaim>> &_handlerClaims, const std::vector<HandlerClaimState> &_handlerClaimStates):
    handlerClaims(_handlerClaims),
    handlerClaimStates(_handlerClaimStates)
{
}

State State::ClaimAllSockets() const
{
    std::vector<std::shared_ptr<HandlerClaim>> newHandlerClaims;
    std::vector<HandlerClaimState> newHandlerClaimStates;
    for(auto &claim:handlerClaims)
    {
        newHandlerClaimStates.push_back(claim->Claim());
        newHandlerClaims.push_back(claim);
    }
    return State(newHandlerClaims,newHandlerClaimStates);
}

State State::DisclaimAllSockets() const
{
    std::vector<std::shared_ptr<HandlerClaim>> newHandlerClaims;
    std::vector<HandlerClaimState> newHandlerClaimStates;
    for(auto &claim:handlerClaims)
    {
        newHandlerClaimStates.push_back(claim->Disclaim());
        newHandlerClaims.push_back(claim);
    }
    return State(newHandlerClaims,newHandlerClaimStates);
}

State State::AddSocket(int fd) const
{
    //copy old claim and it's current states
    std::vector<std::shared_ptr<HandlerClaim>> newHandlerClaims;
    std::vector<HandlerClaimState> newHandlerClaimStates;
    for(auto &oldClaim:handlerClaims)
    {
        newHandlerClaims.push_back(oldClaim);
        newHandlerClaimStates.push_back(oldClaim->GetState());
    }
    //create new claim
    auto newClaim=std::make_shared<HandlerClaim>(HandlerClaim(fd));
    newHandlerClaims.push_back(newClaim);
    newHandlerClaimStates.push_back(newClaim->GetState());
    return State(newHandlerClaims,newHandlerClaimStates);
}

State State::AddSocketWithClaim(int fd) const
{
    //copy old claim and it's current states
    std::vector<std::shared_ptr<HandlerClaim>> newHandlerClaims;
    std::vector<HandlerClaimState> newHandlerClaimStates;
    for(auto &oldClaim:handlerClaims)
    {
        newHandlerClaims.push_back(oldClaim);
        newHandlerClaimStates.push_back(oldClaim->GetState());
    }
    //create new claim
    auto newClaim=std::make_shared<HandlerClaim>(HandlerClaim(fd));
    newHandlerClaims.push_back(newClaim);
    newHandlerClaimStates.push_back(newClaim->Claim());
    return State(newHandlerClaims,newHandlerClaimStates);
}
