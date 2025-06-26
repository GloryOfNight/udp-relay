// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/platform.h"

#if PLATFORM_WINDOWS
#include <WinSock2.h>
#elif PLATFORM_LINUX
#include <netinet/in.h>
#endif

#include <cstdint>
#include <memory>
#include <string>

// address class for sockets to handle ip & ports
struct internetaddr final
{
	internetaddr() noexcept;
	internetaddr(const sockaddr_in& addr) noexcept;

	// raw ip. Network byte order
	int32_t getIp() const noexcept;

	// set raw ip. Network byte order
	void setIp(const int32_t ip) noexcept;

	// get port. Network byte order
	uint16_t getPort() const noexcept;

	// set port. Network byte order
	void setPort(const uint16_t port) noexcept;

	// get current address as a address string
	std::string toString() const;

	// get native addr structure
	const sockaddr_in& getAddr() const noexcept { return m_addr; };

	bool operator==(const internetaddr& other) const noexcept { return getIp() == other.getIp() && getPort() == other.getPort(); }
	bool operator!=(const internetaddr& other) const noexcept { return getIp() != other.getIp() || getPort() != other.getPort(); }

	// check if is default constructed
	bool isValid() const noexcept { return *this != internetaddr(); };

private:
	sockaddr_in m_addr{};
};

using uniqueInternetaddr = std::unique_ptr<internetaddr>;
using sharedInternetaddr = std::shared_ptr<internetaddr>;