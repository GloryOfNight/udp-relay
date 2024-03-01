#pragma once

#include "socket/udpsocket.hxx"

class udpsocketUnix : public udpsocket
{
public:
	udpsocketUnix();

	virtual ~udpsocketUnix();

    virtual bool bind(int32_t port) override;

	virtual int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr* addr) override;

	virtual int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr* addr) override;

	virtual bool setNonBlocking(bool value) override;

	virtual bool isValid() override;

private:
	int32_t m_socket{-1};
};