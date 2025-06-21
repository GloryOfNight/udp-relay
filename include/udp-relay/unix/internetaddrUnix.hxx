// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string>

struct internetaddrUnix
{
	internetaddrUnix();

	internetaddrUnix(const sockaddr_in& addr);

	int32_t getIp() const;
	void setIp(const int32_t ip);

	uint16_t getPort() const;
	void setPort(const uint16_t port);

	std::string toString() const;

	const sockaddr_in& getAddr() const { return m_addr; };

	bool operator==(const internetaddrUnix& other) const { return getIp() == other.getIp() && getPort() == other.getPort(); }
	bool operator!=(const internetaddrUnix& other) const { return getIp() != other.getIp() || getPort() != other.getPort(); }

	bool isValid() const { return *this != internetaddrUnix(); };

private:
	sockaddr_in m_addr{};
};
