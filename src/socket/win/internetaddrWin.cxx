#include "udp-relay/win/internetaddrWin.hxx"

#include <format>
#include <ws2tcpip.h>

internetaddrWin::internetaddrWin()
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = 0;
}

int32_t internetaddrWin::getIp() const
{
	return m_addr.sin_addr.s_addr;
}

void internetaddrWin::setIp(const int32_t ip)
{
	m_addr.sin_addr.s_addr = ip;
}

uint16_t internetaddrWin::getPort() const
{
	return m_addr.sin_port;
}

void internetaddrWin::setPort(const uint16_t port)
{
	m_addr.sin_port = port;
}

std::string internetaddrWin::toString() const
{
	char ipBuffer[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(m_addr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
	return std::format("{0}:{1}", ipBuffer, ntohs(m_addr.sin_port));
}
