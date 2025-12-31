// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

// socket for UDP messaging
class udpsocket final
{
public:
	// handle for native sockets descriptors
#if UR_PLATFORM_WINDOWS
	using socket_t = uint64_t;
#elif UR_PLATFORM_LINUX
	using socket_t = int;
#endif

	udpsocket() noexcept;

	udpsocket(const udpsocket&) = delete;
	udpsocket& operator=(const udpsocket&) = delete;

	udpsocket(udpsocket&&) noexcept;
	udpsocket& operator=(udpsocket&&) noexcept;

	~udpsocket() noexcept;

	// create new socket, result must be checked if valid
	static udpsocket make(bool makeIpv6) noexcept;

	static int32_t getLastErrno() noexcept;

	// closes and invalidates current socket object
	void close() noexcept;

	// bind on socket on specific port in host byte order
	bool bind(const struct socket_address& addr) const;

	// get socket port in host byte order
	uint16_t getPort() const;

	// get raw socket handle
	socket_t getNativeSocket() const noexcept;

	// true if socket handle is valid and ready to use
	bool isValid() const noexcept;

	// return true if ipv6 enabled
	bool isIpv6() const noexcept;

	// sends data to addr. Return bytes sent or -1 on error
	int32_t sendTo(void* buffer, size_t bufferSize, const struct socket_address& addr) const noexcept;

	// receives data. Return bytes received or -1 on error
	int32_t recvFrom(void* buffer, size_t bufferSize, struct socket_address& addr) const noexcept;

	// for ipv6 socket, set if socket should be ipv6 only or dual-stack
	bool setOnlyIpv6(bool value) const noexcept;

	// allow socket to reuse addr
	bool setReuseAddr(bool bAllowReuse = true) const noexcept;

	// set socket non-blocking behavior
	bool setNonBlocking(bool bNonBlocking = true) const noexcept;

	// sets send buffer size. Returns true on success and sets newSize to actual buffer size applied.
	bool setSendBufferSize(int32_t size) const noexcept;

	// return send buffer size
	int32_t getSendBufferSize() const noexcept;

	// sets recv buffer size. Returns true on success and sets newSize to actual buffer size applied.
	bool setRecvBufferSize(int32_t size) const noexcept;

	// return recv buffer size
	int32_t getRecvBufferSize() const noexcept;

	// set send operation timeout, used in blocking sockets. Return true on success.
	bool setSendTimeoutUs(std::chrono::microseconds timeout) const noexcept;

	// set receive operation timeout, used in blocking sockets. Return true on success.
	bool setRecvTimeoutUs(std::chrono::microseconds timeout) const noexcept;

	// wait for incoming data. Return true if data available.
	bool waitForRead(std::chrono::microseconds timeout) const noexcept;

	// wait socket ready to write state. Return true if ready.
	bool waitForWrite(std::chrono::microseconds timeout) const noexcept;

private:
	socket_t m_socket;
};