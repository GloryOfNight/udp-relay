#pragma once

#include <cstdint>
#include <string>
struct internetaddrWin
{
	internetaddrWin() = default;
	internetaddrWin(const internetaddrWin&) = delete;
	internetaddrWin(internetaddrWin&&) = delete;
	~internetaddrWin() = default;

	int32_t getIp() const { return 0; };
	void setIp(const int32_t ip){};

	uint16_t getPort() const { return 0; };
	void setPort(const uint16_t port){};

	std::string toString() const { return std::string("unimplemented"); };

	bool operator==(const internetaddrWin& other) const { return getIp() == other.getIp() && getPort() == other.getPort(); }
};