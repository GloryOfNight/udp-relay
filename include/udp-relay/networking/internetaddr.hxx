// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/reldefs.h"

#if PLATFORM_WINDOWS
#include <WinSock2.h>
#elif PLATFORM_LINUX
#include <netinet/in.h>
#endif

#include <cstdint>
#include <memory>
#include <string>

struct internetaddr
{
	internetaddr();

	internetaddr(const struct sockaddr_in& addr);

	int32_t getIp() const;
	void setIp(const int32_t ip);

	uint16_t getPort() const;
	void setPort(const uint16_t port);

	std::string toString() const;

	const sockaddr_in& getAddr() const { return m_addr; };

	bool operator==(const internetaddr& other) const { return getIp() == other.getIp() && getPort() == other.getPort(); }
	bool operator!=(const internetaddr& other) const { return getIp() != other.getIp() || getPort() != other.getPort(); }

	bool isValid() const { return *this != internetaddr(); };

private:
	sockaddr_in m_addr{};
};

using uniqueInternetaddr = std::unique_ptr<internetaddr>;
using sharedInternetaddr = std::shared_ptr<internetaddr>;