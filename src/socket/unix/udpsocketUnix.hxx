#pragma once

#include <cstdint>
#include <stddef.h>

struct internetaddrUnix;
using internetaddr = internetaddrUnix;

class udpsocketUnix
{
public:
	udpsocketUnix();
	udpsocketUnix(const udpsocketUnix&) = delete;
	udpsocketUnix(udpsocketUnix&&) = delete;
	~udpsocketUnix();

	bool bind(int32_t port);

	int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr* addr);

	int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr* addr);

	bool setNonBlocking(bool bNonBlocking);

	bool setSendBufferSize(int32_t size, int32_t& newSize);

	bool setRecvBufferSize(int32_t size, int32_t& newSize);

	bool waitForRead(int32_t timeoutms);

	bool waitForWrite(int32_t timeoutms);

	bool isValid();

private:
	int32_t m_socket{-1};
};