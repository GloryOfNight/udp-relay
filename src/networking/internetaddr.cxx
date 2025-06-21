// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/networking/internetaddr.hxx"

#if PLATFORM_WINDOWS
#include <ws2tcpip.h>
#elif PLATFORM_LINUX
#include <arpa/inet.h>
#endif

#include <format>

internetaddr::internetaddr()
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = 0;
}

internetaddr::internetaddr(const sockaddr_in& addr)
	: m_addr{addr}
{
}

int32_t internetaddr::getIp() const
{
	return m_addr.sin_addr.s_addr;
}

void internetaddr::setIp(const int32_t ip)
{
	m_addr.sin_addr.s_addr = ip;
}

uint16_t internetaddr::getPort() const
{
	return m_addr.sin_port;
}

void internetaddr::setPort(const uint16_t port)
{
	m_addr.sin_port = port;
}

std::string internetaddr::toString() const
{
#if PLATFORM_WINDOWS
	char ipBuffer[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(m_addr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
	return std::format("{0}:{1}", ipBuffer, ntohs(m_addr.sin_port));
#elif PLATFORM_LINUX
	return std::format("{0}:{1}", inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
#endif
}