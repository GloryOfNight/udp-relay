#include "udpsocketFactory.hxx"

#include "unix/internetaddrUnix.hxx"
#include "unix/udpsocketUnix.hxx"

std::unique_ptr<udpsocket> udpsocketFactory::createUdpSocket()
{
	return std::unique_ptr<udpsocket>(new udpsocketUnix());
}

std::unique_ptr<internetaddr> udpsocketFactory::createInternetAddrUnique()
{
    return std::unique_ptr<internetaddr>(new internetaddrUnix());
}

std::shared_ptr<internetaddr> udpsocketFactory::createInternetAddr()
{
    return std::shared_ptr<internetaddr>(new internetaddrUnix());
}