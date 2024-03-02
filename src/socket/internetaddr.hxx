#pragma once

#include <cstdint>
#include <string>
struct internetaddr
{
	virtual ~internetaddr() = default;

	virtual int32_t getIp() const = 0;
	virtual void setIp(const int32_t ip) = 0;

	virtual uint16_t getPort() const = 0;
	virtual void setPort(const uint16_t port) = 0;

	virtual std::string toString() const = 0;

	virtual bool operator==(const internetaddr&) const = 0;
};