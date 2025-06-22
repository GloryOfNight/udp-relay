// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/platform.h"

#include <cstdint>
#include <memory>
#include <stddef.h>

using socket_t = int64_t;

class udpsocket final
{
public:
	udpsocket();
	udpsocket(const udpsocket&) = delete;
	udpsocket(udpsocket&&) = delete;
	~udpsocket();

	// bind on socket on specific port in host byte order
	bool bind(int32_t port) const;

	// @return port in host byte order
	uint16_t getPort() const;

	int32_t sendTo(void* buffer, size_t bufferSize, const struct internetaddr* addr) const;

	int32_t recvFrom(void* buffer, size_t bufferSize, struct internetaddr* addr) const;

	bool setReuseAddr(bool bAllowReuse) const;

	bool setNonBlocking(bool bNonBlocking) const;

	bool setSendBufferSize(int32_t size, int32_t& newSize) const;

	bool setRecvBufferSize(int32_t size, int32_t& newSize) const;

	bool setRecvTimeoutUs(int64_t timeoutUs) const;

	bool setSendTimeoutUs(int64_t timeoutUs) const;

	bool waitForReadUs(int64_t timeoutUs) const;

	bool waitForWriteUs(int64_t timeoutUs) const;

	bool isValid() const;

private:
	socket_t m_socket{static_cast<socket_t>(-1)};
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