#pragma once

#include <memory>

#ifndef SE_WOULDBLOCK
#define SE_WOULDBLOCK EWOULDBLOCK
#endif

#if __unix
#include "unix/udpsocketUnix.hxx"
using udpsocket = udpsocketUnix;
#elif _WIN32
#include "win/udpsocketWin.hxx"
using udpsocket = udpsocketWin;
#endif

using uniqueUdpsocket = std::unique_ptr<udpsocket>;

class udpsocketFactory
{
public:
	static uniqueUdpsocket createUdpSocket()
	{
		return uniqueUdpsocket(new udpsocket());
	}
};