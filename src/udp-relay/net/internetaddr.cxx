// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/net/internetaddr.hxx"

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

internetaddr internetaddr::make_ipv4(uint32_t ip, uint16_t port) noexcept
{
	internetaddr newAddr{};
	newAddr.m_family = AF_INET;
	newAddr.m_port = port;
	std::memcpy(newAddr.m_ip.data(), &ip, sizeof(uint32_t));
	return newAddr;
}

internetaddr internetaddr::make_ipv6(std::array<std::byte, 16> ip, uint16_t port) noexcept
{
	internetaddr newAddr{};
	newAddr.m_family = AF_INET6;
	newAddr.m_port = port;
	newAddr.m_ip = std::move(ip);
	return newAddr;
}

bool internetaddr::isNull() const noexcept
{
	internetaddr zeroAddr{};
	return std::memcmp(this, &zeroAddr, sizeof(internetaddr)) == 0;
}

void internetaddr::copyToNative(sockaddr_storage& saddr) const noexcept
{
	if (m_family == AF_INET6)
	{
		auto v6 = reinterpret_cast<sockaddr_in6*>(&saddr);
		v6->sin6_family = AF_INET6;
		v6->sin6_port = ur::net::hton16(m_port);

		static_assert(sizeof(internetaddr::m_ip) == sizeof(v6->sin6_addr));
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

void internetaddr::copyFromNative(const sockaddr_storage& saddr) noexcept
{
	if (saddr.ss_family == AF_INET6)
	{
		auto v6 = reinterpret_cast<const sockaddr_in6*>(&saddr);
		m_family = v6->sin6_family;
		m_port = ur::net::ntoh16(v6->sin6_port);

		static_assert(sizeof(internetaddr::m_ip) == sizeof(v6->sin6_addr));
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

std::string internetaddr::toString(bool withPort) const
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

bool internetaddr::operator==(const internetaddr& other) const noexcept
{
	return std::memcmp(this, &other, sizeof(internetaddr)) == 0;
}

bool internetaddr::operator!=(const internetaddr& other) const noexcept
{
	return std::memcmp(this, &other, sizeof(internetaddr)) != 0;
}

const std::array<std::byte, 16>& internetaddr::getRawIp() const noexcept
{
	return m_ip;
}

uint16_t internetaddr::getPort() const noexcept
{
	return m_port;
}

bool internetaddr::isIpv4() const noexcept
{
	return m_family == AF_INET;
}

bool internetaddr::isIpv6() const noexcept
{
	return m_family == AF_INET6;
}