#pragma once

#include "internetaddr.hxx"
#include "udpsocket.hxx"

#include <memory>

class udpsocketFactory
{
public:
	static uniqueUdpsocket createUdpSocket()
	{
		return uniqueUdpsocket(new udpsocket());
	}

	static uniqueInternetaddr createInternetAddrUnique()
	{
		return uniqueInternetaddr(new internetaddr());
	}

	static sharedInternetaddr createInternetAddr()
	{
		return sharedInternetaddr(new internetaddr());
	}
};