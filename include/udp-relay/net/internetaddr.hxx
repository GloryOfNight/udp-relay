// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

struct sockaddr_storage;

namespace ur::net
{
	extern uint32_t anyIpv4();
	extern uint32_t localhostIpv4();

	extern std::array<std::byte, 16> anyIpv6();
	extern std::array<std::byte, 16> localhostIpv6();
} // namespace ur::net

// address class for sockets to handle ip & ports
struct internetaddr final
{
	// make ipv4 address
	static internetaddr make_ipv4(uint32_t ip, uint16_t port) noexcept;

	// make ipv6 address
	static internetaddr make_ipv6(std::array<std::byte, 16> ip, uint16_t port) noexcept;

	// convert address to address string
	std::string toString(bool withPort = true) const;

	// copy address from native address structure
	void copyToNative(sockaddr_storage& saddr) const noexcept;

	// copy address to native address structure
	void copyFromNative(const sockaddr_storage& saddr) noexcept;

	// check is initialized
	bool isNull() const noexcept;

	// get raw ip array
	const std::array<std::byte, 16>& getRawIp() const noexcept;

	// get host ordered port
	uint16_t getPort() const noexcept;

	// true if initialized as ipv4
	bool isIpv4() const noexcept;

	// true if initialized as ipv6
	bool isIpv6() const noexcept;

	bool operator==(const internetaddr& other) const noexcept;
	bool operator!=(const internetaddr& other) const noexcept;

private:
	uint16_t m_family{};						 // AF_INET or AF_INET6
	uint16_t m_port{};							 // host order
	alignas(4) std::array<std::byte, 16> m_ip{}; // ip bytes
};

namespace std
{
	template <>
	struct hash<internetaddr>
	{
		std::size_t operator()(const internetaddr& val) const noexcept
		{
			const auto& rawIp = val.getRawIp();
			const uint32_t* a = reinterpret_cast<const uint32_t*>(rawIp.data());
			const uint32_t* b = reinterpret_cast<const uint32_t*>(rawIp.data() + 4);
			const uint32_t* c = reinterpret_cast<const uint32_t*>(rawIp.data() + 8);
			const uint32_t* d = reinterpret_cast<const uint32_t*>(rawIp.data() + 12);
			const uint16_t port = val.getPort();

			size_t h = std::hash<uint32_t>{}(*a);
			h = h * 31 + std::hash<uint32_t>{}(*b);
			h = h * 31 + std::hash<uint32_t>{}(*c);
			h = h * 31 + std::hash<uint32_t>{}(*d);
			h = h * 31 + std::hash<uint16_t>{}(port);

			return h;
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
			const int ipCmp = std::memcmp(left.getRawIp().data(), right.getRawIp().data(), left.getRawIp().size());
			if (ipCmp != 0)
				return ipCmp < 0;
			return left.getPort() < right.getPort();
		}
	};
} // namespace std