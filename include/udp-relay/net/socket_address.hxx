// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <string>

using native_socket_address = struct sockaddr_storage;

namespace ur::net
{
	extern uint32_t anyIpv4();
	extern uint32_t localhostIpv4();

	extern std::array<std::byte, 16> anyIpv6();
	extern std::array<std::byte, 16> localhostIpv6();
} // namespace ur::net

// address class for sockets to handle ip & ports
struct socket_address final
{
	// make ipv4 address
	static socket_address make_ipv4(uint32_t ip, uint16_t port) noexcept;

	// make ipv6 address
	static socket_address make_ipv6(std::array<std::byte, 16> ip, uint16_t port) noexcept;

	// convert address to address string
	std::string toString(bool withPort = true) const;

	// copy address from native address structure
	void copyToNative(native_socket_address& saddr) const noexcept;

	// copy address to native address structure
	void copyFromNative(const native_socket_address& saddr) noexcept;

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

	bool operator==(const socket_address& other) const noexcept;
	bool operator!=(const socket_address& other) const noexcept;

private:
	uint16_t m_family{};						 // AF_INET or AF_INET6
	uint16_t m_port{};							 // host order
	alignas(4) std::array<std::byte, 16> m_ip{}; // ip bytes
};

namespace std
{
	template <>
	struct hash<socket_address>
	{
		std::size_t operator()(const socket_address& val) const noexcept
		{
			alignas(4) std::array<std::byte, 16 + 2> ipPort{};

			std::memcpy(ipPort.data(), val.getRawIp().data(), 16);
			uint16_t port = val.getPort();
			std::memcpy(ipPort.data() + 16, &port, sizeof(port));

			return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(ipPort.data()), ipPort.size()));
		}
	};

	template <>
	struct equal_to<socket_address>
	{
		bool operator()(const socket_address& left, const socket_address& right) const noexcept
		{
			return left == right;
		}
	};

	template <>
	struct less<socket_address>
	{
		bool operator()(const socket_address& left, const socket_address& right) const noexcept
		{
			const int ipCmp = std::memcmp(left.getRawIp().data(), right.getRawIp().data(), left.getRawIp().size());
			return ipCmp != 0 ? ipCmp < 0 : left.getPort() < right.getPort();
		}
	};
} // namespace std

template <>
struct std::formatter<socket_address>
{
private:
	char m_formatMode = 'D';

public:
	constexpr auto parse(std::format_parse_context& ctx)
	{
		auto it = ctx.begin();

		if (it != ctx.end())
		{
			char c = *it;
			if (c == 'D' || c == 'A' || c == 'P')
			{
				m_formatMode = c;
				++it;
			}
		}

		return it;
	}

	template <typename FormatContext>
	auto format(const socket_address& value, FormatContext& ctx) const
	{
		switch (m_formatMode)
		{
		case 'A': // address only
			return std::format_to(ctx.out(), "{}", value.toString(false));
		case 'P': // port only
			return std::format_to(ctx.out(), "{}", value.getPort());
		default: // address:port
			return std::format_to(ctx.out(), "{}", value.toString());
		}
	}
};