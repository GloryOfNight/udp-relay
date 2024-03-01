#include "udpsocketFactory.hxx"

#include "unix/udpsocketUnix.hxx"
#include "unix/internetaddrUnix.hxx"

std::unique_ptr<udpsocket> udpsocketFactory::createUdpSocket(const int32_t port)
{
    return nullptr;
}