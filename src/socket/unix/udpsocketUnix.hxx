#pragma once

#include "socket/udpsocket.hxx"

#if __unix

class udpsocketUnix : public udpsocket
{
public:
	udpsocketUnix();

	virtual ~udpsocketUnix();

	virtual bool bind(int32_t port) override;

	virtual int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr* addr) override;

	virtual int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr* addr) override;

	virtual bool setNonBlocking(bool bNonBlocking) override;

	virtual bool setSendBufferSize(int32_t size, int32_t& newSize) override;

	virtual bool setRecvBufferSize(int32_t size, int32_t& newSize) override;

	virtual bool isValid() override;

private:
	int32_t m_socket{-1};
};

#endif