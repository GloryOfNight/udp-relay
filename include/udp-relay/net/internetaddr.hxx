// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#if UR_PLATFORM_WINDOWS
#include <WinSock2.h>
#elif UR_PLATFORM_LINUX
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

namespace std
{
	template <>
	struct hash<internetaddr>
	{
		std::size_t operator()(const internetaddr& g) const noexcept
		{
			const uint64_t value = (static_cast<uint64_t>(g.getIp()) << 32) | static_cast<uint64_t>(g.getPort());
			return std::hash<uint64_t>{}(value);
		}
	};

	template <>
	struct equal_to<internetaddr>
	{
		bool operator()(const internetaddr& left, const internetaddr& right) const noexcept
		{
			return left == right;
		}
	};

	template <>
	struct less<internetaddr>
	{
		bool operator()(const internetaddr& left, const internetaddr& right) const noexcept
		{
			if (left.getIp() != right.getIp())
				return left.getIp() < right.getIp();
			return left.getPort() < right.getPort();
		}
	};
} // namespace std