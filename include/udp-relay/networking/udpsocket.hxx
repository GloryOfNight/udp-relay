// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/reldefs.h"

#include <cstdint>
#include <memory>
#include <stddef.h>

#ifndef SE_WOULDBLOCK
#define SE_WOULDBLOCK EWOULDBLOCK
#endif

struct internetaddr;

class udpsocket final
{
public:
	udpsocket();
	udpsocket(const udpsocket&) = delete;
	udpsocket(udpsocket&&) = delete;
	~udpsocket();

	// bind on socket on specific port in host byte order
	bool bind(int32_t port);

	int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr* addr);

	int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr* addr);

	// @return port in host byte order
	uint16_t getPort() const;

	bool setNonBlocking(bool bNonBlocking);

	bool setSendBufferSize(int32_t size, int32_t& newSize);

	bool setRecvBufferSize(int32_t size, int32_t& newSize);

	bool waitForReadUs(int32_t timeoutUs);

	bool waitForWriteUs(int32_t timeoutUs);

	bool isValid() const;

private:
#if PLATFORM_WINDOWS
	using SOCKET = unsigned int;
#elif PLATFORM_LINUX
	using SOCKET = int;
#endif

	SOCKET m_socket{static_cast<SOCKET>(-1)};
};

using uniqueUdpsocket = std::unique_ptr<udpsocket>;

class udpsocketFactory
{
public:
	static uniqueUdpsocket createUdpSocket()
	{
		return uniqueUdpsocket(new udpsocket());
	}
};