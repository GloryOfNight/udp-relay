// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/platform.h"

#include <cstdint>
#include <memory>
#include <stddef.h>

// handle for native sockets descriptors
using socket_t = int64_t;

// socket for UDP messaging
class udpsocket final
{
public:
	udpsocket();
	udpsocket(const udpsocket&) = delete;
	udpsocket(udpsocket&&) = delete;
	~udpsocket();

	// bind on socket on specific port in host byte order
	bool bind(int32_t port) const;

	// get socket port in host byte order
	uint16_t getPort() const;

	// get raw socket handle
	socket_t getNativeSocket() const;

	// true if socket handle is valid and class ready to use
	bool isValid() const;

	// sends data to addr. Return bytes sent or -1 on error
	int32_t sendTo(void* buffer, size_t bufferSize, const struct internetaddr* addr) const;

	//  receives data. Return bytes received or -1 on error
	int32_t recvFrom(void* buffer, size_t bufferSize, struct internetaddr* addr) const;

	// allow socket to reuse addr
	bool setReuseAddr(bool bAllowReuse = true) const;

	// set socket non-blocking behavior
	bool setNonBlocking(bool bNonBlocking = true) const;

	// sets send buffer size. Returns true on success and sets newSize to actual buffer size applied.
	bool setSendBufferSize(int32_t size, int32_t& newSize) const;

	// sets recv buffer size. Returns true on success and sets newSize to actual buffer size applied.
	bool setRecvBufferSize(int32_t size, int32_t& newSize) const;

	// set send operation timeout, used in blocking sockets. Return true on success.
	bool setSendTimeoutUs(int64_t timeoutUs) const;

	// set receive operation timeout, used in blocking sockets. Return true on success.
	bool setRecvTimeoutUs(int64_t timeoutUs) const;

	// wait for incoming data. Return true if data available.
	bool waitForReadUs(int64_t timeoutUs) const;

	// wait socket ready to write state. Return true if ready.
	bool waitForWriteUs(int64_t timeoutUs) const;

private:
	socket_t m_socket{static_cast<socket_t>(-1)};
};

using uniqueUdpsocket = std::unique_ptr<udpsocket>;

class udpsocketFactory
{
public:
	// creates unique udp socket handle
	static uniqueUdpsocket createUdpSocket()
	{
		return uniqueUdpsocket(new udpsocket());
	}
};