#pragma once

#include "internetaddr.hxx"

#include <cstdint>
#include <memory>

class udpsocket
{
public:
	virtual ~udpsocket() = default;

	virtual int32_t sendTo(void* buffer, size_t bufferSize, const internetaddr& addr) = 0;

	virtual int32_t recvFrom(void* buffer, size_t bufferSize, internetaddr& addr) = 0;

	virtual bool setNonBlocking(bool value) = 0;
};