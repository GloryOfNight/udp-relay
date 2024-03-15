#pragma once

#include <WinSock2.h>
#include <cstdint>
#include <string>

struct internetaddrWin
{
	internetaddrWin();
	internetaddrWin(const internetaddrWin&) = delete;
	internetaddrWin(internetaddrWin&&) = delete;
	~internetaddrWin() = default;

	int32_t getIp() const;
	void setIp(const int32_t ip);

	uint16_t getPort() const;
	void setPort(const uint16_t port);

	std::string toString() const;

	const sockaddr_in& getAddr() const { return m_addr; };

	bool operator==(const internetaddrWin& other) const { return getIp() == other.getIp() && getPort() == other.getPort(); }

private:
	sockaddr_in m_addr{};
};