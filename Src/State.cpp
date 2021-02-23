#include "State.h"

void State::CopyClaims(std::unordered_map<int, SocketClaim*> &target)
{
    for(auto &claim:socketClaims)
        target.insert({claim.first,claim.second});
}
