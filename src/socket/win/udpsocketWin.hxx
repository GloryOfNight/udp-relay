#pragma once

#include <cstdint>

struct internetaddrWin;
using internetaddr = internetaddrWin;

// unimplemented
class udpsocketWin
{
public:
	udpsocketWin();
	udpsocketWin(const udpsocketWin&) = delete;
	udpsocketWin(udpsocketWin&&) = delete;
	~udpsocketWin();

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
	using SOCKET = unsigned int;
	SOCKET m_socket{static_cast<SOCKET>(-1)};
};