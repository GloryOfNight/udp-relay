// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/networking/internetaddr.hxx"

#include "udp-relay/networking/network_utils.hxx"

#if PLATFORM_WINDOWS
#include <ws2tcpip.h>
#elif PLATFORM_LINUX
#include <arpa/inet.h>
#endif

#include <format>

internetaddr::internetaddr() noexcept
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = 0;
}

internetaddr::internetaddr(const sockaddr_in& addr) noexcept
	: m_addr{addr}
{
}

int32_t internetaddr::getIp() const noexcept
{
	return m_addr.sin_addr.s_addr;
}

void internetaddr::setIp(const int32_t ip) noexcept
{
	m_addr.sin_addr.s_addr = ip;
}

uint16_t internetaddr::getPort() const noexcept
{
	return m_addr.sin_port;
}

void internetaddr::setPort(const uint16_t port) noexcept
{
	m_addr.sin_port = port;
}

std::string internetaddr::toString() const
{
#if PLATFORM_WINDOWS
	char ipBuffer[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(m_addr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
	return std::format("{0}:{1}", ipBuffer, ur::net::ntoh16(m_addr.sin_port));
#elif PLATFORM_LINUX
	return std::format("{0}:{1}", inet_ntoa(m_addr.sin_addr), ur::net::ntoh16(m_addr.sin_port));
#endif
}