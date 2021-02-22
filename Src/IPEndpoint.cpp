#include "IPEndpoint.h"

IPEndpoint::IPEndpoint():
    address(IPAddress()),
    port(0)
{
}

IPEndpoint::IPEndpoint(const IPAddress &_address, const ushort _port):
    address(_address),
    port(_port)
{
}

IPEndpoint::IPEndpoint(const IPEndpoint& other):
    address(other.address),
    port(other.port)
{
}

size_t IPEndpoint::GetHashCode() const
{
    return address.GetHashCode() ^ static_cast<size_t>(port);
}

bool IPEndpoint::Equals(const IPEndpoint& other) const
{
    return address.Equals(other.address) && port==other.port;
}

bool IPEndpoint::Less(const IPEndpoint& other) const
{
    if(address.Less(other.address))
        return true;
    if(port<other.port)
        return true;
    return false;
}

bool IPEndpoint::Greater(const IPEndpoint& other) const
{
    if(address.Greater(other.address))
        return true;
    if(port>other.port)
        return true;
    return false;
}

bool IPEndpoint::operator<(const IPEndpoint& other) const
{
    return Less(other);
}

bool IPEndpoint::operator==(const IPEndpoint& other) const
{
    return Equals(other);
}

bool IPEndpoint::operator>(const IPEndpoint& other) const
{
    return Greater(other);
}

bool IPEndpoint::operator>=(const IPEndpoint& other) const
{
    return Greater(other)||Equals(other);
}

bool IPEndpoint::operator<=(const IPEndpoint& other) const
{
    return Less(other)||Equals(other);
}

std::ostream& operator<<(std::ostream& stream, const IPEndpoint& target)
{
    stream << target.address << ":" << target.port;
    return stream;
}

