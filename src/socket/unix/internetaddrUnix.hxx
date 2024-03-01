#pragma once

#include "socket/internetaddr.hxx"

#include <netinet/in.h>

struct internetaddrUnix : public internetaddr
{
    internetaddrUnix();

    internetaddrUnix(const sockaddr_in& addr);

	virtual int32_t getIp() const override;
	virtual void setIp(const int32_t ip) override;

	virtual uint16_t getPort() const override;
	virtual void setPort(const uint16_t port) override;

	const sockaddr_in& getAddr() const { return m_addr; };

private:
	sockaddr_in m_addr{};
};