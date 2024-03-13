#pragma once

#include <cstdint>

struct internetaddrWin;
using internetaddr = internetaddrWin;

// unimplemented
class udpsocketWin
{
public:
	udpsocketWin() = default;
	udpsocketWin(const udpsocketWin&) = delete;
	udpsocketWin(udpsocketWin&&) = delete;
	~udpsocketWin() = default;

	bool bind(int32_t port) { return false; };

	int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr* addr) { return -1; };

	int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr* addr) { return -1; };

	bool setNonBlocking(bool value) { return false; };

	bool setSendBufferSize(int32_t size, int32_t& newSize) { return false; };

	bool setRecvBufferSize(int32_t size, int32_t& newSize) { return false; };

	bool waitForRead(int32_t timeoutms) { return false; };

	bool waitForWrite(int32_t timeoutms) { return false; };

	bool isValid() { return false; };
};