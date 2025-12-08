// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

import std.compat;

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
			alignas(4) std::array<std::byte, 16 + 2> ipPort{};

			std::memcpy(ipPort.data(), val.getRawIp().data(), 16);
			uint16_t port = val.getPort();
			std::memcpy(ipPort.data() + 16, &port, sizeof(port));

			return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(ipPort.data()), ipPort.size()));
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
			return ipCmp != 0 ? ipCmp < 0 : left.getPort() < right.getPort();
		}
	};
} // namespace std