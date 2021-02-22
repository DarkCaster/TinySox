#ifndef IPENDPOINT_H
#define IPENDPOINT_H

#include "IPAddress.h"

class IPEndpoint
{
    public:
        IPEndpoint();
        IPEndpoint(const IPAddress &address, const ushort port);
        IPEndpoint(const IPEndpoint &other);

        const IPAddress address;
        const ushort port;

        size_t GetHashCode() const;
        bool Equals(const IPEndpoint &other) const;
        bool Less(const IPEndpoint &other) const;
        bool Greater(const IPEndpoint &other) const;
        bool operator<(const IPEndpoint &other) const;
        bool operator==(const IPEndpoint &other) const;
        bool operator>(const IPEndpoint &other) const;
        bool operator>=(const IPEndpoint &other) const;
        bool operator<=(const IPEndpoint &other) const;
        friend std::ostream& operator<<(std::ostream& stream, const IPEndpoint& target);
};

namespace std { template<> struct hash<IPEndpoint>{ size_t operator()(const IPEndpoint &target) const {return target.GetHashCode();}}; }

#endif //IPENDPOINT_H
