#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string>

struct internetaddrUnix
{
	internetaddrUnix();
	internetaddrUnix(const internetaddrUnix&) = delete;
	internetaddrUnix(internetaddrUnix&&) = delete;
	~internetaddrUnix() = default;

	internetaddrUnix(const sockaddr_in& addr);

	int32_t getIp() const;
	void setIp(const int32_t ip);

	uint16_t getPort() const;
	void setPort(const uint16_t port);

	std::string toString() const;

	const sockaddr_in& getAddr() const { return m_addr; };

	bool operator==(const internetaddrUnix& other) const { return getIp() == other.getIp() && getPort() == other.getPort(); }

private:
	sockaddr_in m_addr{};
};
