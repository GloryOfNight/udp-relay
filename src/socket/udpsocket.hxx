#pragma once

#include "internetaddr.hxx"

#include <cstdint>
#include <memory>

enum class networkProtocol
{
    ipv4,
    ipv6
};

class udpsocket
{
public:
	virtual ~udpsocket() = default;

    virtual bool bind(int32_t port) = 0;

	virtual int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr* addr) = 0;

	virtual int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr* addr) = 0;

	virtual bool setNonBlocking(bool value) = 0;

	virtual bool setSendBufferSize(int32_t size, int32_t& newSize) = 0;

	virtual bool setRecvBufferSize(int32_t size, int32_t& newSize) = 0;

	virtual bool isValid() = 0;
};