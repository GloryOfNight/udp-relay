#include "udpsocketFactory.hxx"

#include "unix/internetaddrUnix.hxx"
#include "unix/udpsocketUnix.hxx"

std::unique_ptr<udpsocket> udpsocketFactory::createUdpSocket()
{
#if __unix
	return std::unique_ptr<udpsocket>(new udpsocketUnix());
#else
	return nullptr;
#endif
}

std::unique_ptr<internetaddr> udpsocketFactory::createInternetAddrUnique()
{
#if __unix
	return std::unique_ptr<internetaddr>(new internetaddrUnix());
#else
	return nullptr;
#endif
}

std::shared_ptr<internetaddr> udpsocketFactory::createInternetAddr()
{
#if __unix
	return std::shared_ptr<internetaddr>(new internetaddrUnix());
#else
	return nullptr;
#endif
}