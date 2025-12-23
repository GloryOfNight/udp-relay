// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/net/socket_address.hxx"

#include "udp-relay/net/network_utils.hxx"

#if UR_PLATFORM_WINDOWS
#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#elif UR_PLATFORM_LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <format>

uint32_t ur::net::anyIpv4()
{
	return INADDR_ANY;
}

uint32_t ur::net::localhostIpv4()
{
	return ur::net::hton32(INADDR_LOOPBACK);
}

std::array<std::byte, 16> ur::net::anyIpv6()
{
	std::array<std::byte, 16> result{};
	return result;
}

std::array<std::byte, 16> ur::net::localhostIpv6()
{
	std::array<std::byte, 16> addr{};
	addr[15] = std::byte{1};
	return addr;
}

socket_address socket_address::make_ipv4(uint32_t ip, uint16_t port) noexcept
{
	socket_address newAddr{};
	newAddr.m_family = AF_INET;
	newAddr.m_port = port;
	std::memcpy(newAddr.m_ip.data(), &ip, sizeof(uint32_t));
	return newAddr;
}

socket_address socket_address::make_ipv6(std::array<std::byte, 16> ip, uint16_t port) noexcept
{
	socket_address newAddr{};
	newAddr.m_family = AF_INET6;
	newAddr.m_port = port;
	newAddr.m_ip = std::move(ip);
	return newAddr;
}

socket_address socket_address::from_string(std::string_view ip)
{
	if (struct in6_addr ipv6; inet_pton(AF_INET6, ip.data(), &ipv6) == 1)
	{
		socket_address result{};
		result.m_family = AF_INET6;
		std::memcpy(result.m_ip.data(), &ipv6, sizeof(ipv6));
		return result;
	}
	else if (struct in_addr ipv4; inet_pton(AF_INET, ip.data(), &ipv4) == 1)
	{
		socket_address result{};
		result.m_family = AF_INET;
		std::memcpy(result.m_ip.data(), &ipv4, sizeof(ipv4));
		return result;
	}
	return socket_address();
}

bool socket_address::isNull() const noexcept
{
	socket_address zeroAddr{};
	return std::memcmp(this, &zeroAddr, sizeof(socket_address)) == 0;
}

void socket_address::copyToNative(native_socket_address& saddr) const noexcept
{
	if (m_family == AF_INET6)
	{
		auto v6 = reinterpret_cast<sockaddr_in6*>(&saddr);
		v6->sin6_family = AF_INET6;
		v6->sin6_port = ur::net::hton16(m_port);

		static_assert(sizeof(socket_address::m_ip) == sizeof(v6->sin6_addr));
		std::memcpy(&v6->sin6_addr, m_ip.data(), sizeof(v6->sin6_addr));
	}
	else if (m_family == AF_INET)
	{
		auto v4 = reinterpret_cast<sockaddr_in*>(&saddr);
		v4->sin_family = AF_INET;
		v4->sin_port = ur::net::hton16(m_port);
		std::memcpy(&v4->sin_addr.s_addr, m_ip.data(), sizeof(uint32_t));
	}
}

void socket_address::copyFromNative(const native_socket_address& saddr) noexcept
{
	if (saddr.ss_family == AF_INET6)
	{
		auto v6 = reinterpret_cast<const sockaddr_in6*>(&saddr);
		m_family = v6->sin6_family;
		m_port = ur::net::ntoh16(v6->sin6_port);

		static_assert(sizeof(socket_address::m_ip) == sizeof(v6->sin6_addr));
		std::memcpy(m_ip.data(), &v6->sin6_addr, sizeof(m_ip));
	}
	else if (saddr.ss_family == AF_INET)
	{
		auto v4 = reinterpret_cast<const sockaddr_in*>(&saddr);
		m_family = v4->sin_family;
		m_port = ur::net::ntoh16(v4->sin_port);
		std::memcpy(m_ip.data(), &v4->sin_addr.s_addr, sizeof(uint32_t));
	}
}

std::string socket_address::toString(bool withPort) const
{
	std::array<char, INET6_ADDRSTRLEN> ipStr{};
	switch (m_family)
	{
	case AF_INET:
	{
		inet_ntop(AF_INET, m_ip.data(), ipStr.data(), ipStr.size());
		return withPort ? std::format("{}:{}", ipStr.data(), m_port) : ipStr.data();
	}

	case AF_INET6:
	{
		inet_ntop(AF_INET6, m_ip.data(), ipStr.data(), ipStr.size());
		return withPort ? std::format("[{}]:{}", ipStr.data(), m_port) : std::format("[{}]", ipStr.data());
	}
	}
	return "invaddr";
}

bool socket_address::operator==(const socket_address& other) const noexcept
{
	return std::memcmp(this, &other, sizeof(socket_address)) == 0;
}

bool socket_address::operator!=(const socket_address& other) const noexcept
{
	return std::memcmp(this, &other, sizeof(socket_address)) != 0;
}

const std::array<std::byte, 16>& socket_address::getRawIp() const noexcept
{
	return m_ip;
}

void socket_address::setPort(uint16_t port) noexcept
{
	m_port = port;
}

uint16_t socket_address::getPort() const noexcept
{
	return m_port;
}

bool socket_address::isIpv4() const noexcept
{
	return m_family == AF_INET;
}

bool socket_address::isIpv6() const noexcept
{
	return m_family == AF_INET6;
}