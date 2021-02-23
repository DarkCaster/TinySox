#ifndef STATE_H
#define STATE_H

#include "SocketClaim.h"
#include <unordered_map>

class State
{
    public:
        void CopyClaims(std::unordered_map<int,SocketClaim*> &target);
        const std::unordered_map<int,SocketClaim*> socketClaims;
        const std::unordered_map<int,const SocketClaimState> socketClaimStates;
};

#endif
