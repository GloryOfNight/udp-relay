// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <WinSock2.h>
#include <cstdint>
#include <string>

struct internetaddrWin
{
	internetaddrWin();

	int32_t getIp() const;
	void setIp(const int32_t ip);

	uint16_t getPort() const;
	void setPort(const uint16_t port);

	std::string toString() const;

	const sockaddr_in& getAddr() const { return m_addr; };

	bool operator==(const internetaddrWin& other) const { return getIp() == other.getIp() && getPort() == other.getPort(); }
	bool operator!=(const internetaddrWin& other) const { return getIp() != other.getIp() || getPort() != other.getPort(); }

	bool isValid() const { return *this != internetaddrWin(); };

private:
	sockaddr_in m_addr{};
};