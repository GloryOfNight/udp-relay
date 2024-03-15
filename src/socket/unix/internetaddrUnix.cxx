#include "udp-relay/unix/internetaddrUnix.hxx"

#include <arpa/inet.h>
#include <format>

internetaddrUnix::internetaddrUnix()
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = 0;
}

internetaddrUnix::internetaddrUnix(const sockaddr_in& addr)
	: m_addr{addr}
{
}

int32_t internetaddrUnix::getIp() const
{
	return m_addr.sin_addr.s_addr;
}

void internetaddrUnix::setIp(const int32_t ip)
{
	m_addr.sin_addr.s_addr = ip;
}

uint16_t internetaddrUnix::getPort() const
{
	return m_addr.sin_port;
}

void internetaddrUnix::setPort(const uint16_t port)
{
	m_addr.sin_port = port;
}

std::string internetaddrUnix::toString() const
{
	return std::format("{0}:{1}", inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
}