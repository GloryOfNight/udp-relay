// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/networking/udpsocket.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/internetaddr.hxx"
#include "udp-relay/networking/network_utils.hxx"

#if PLATFORM_WINDOWS
#include <WinSock2.h>
#elif PLATFORM_LINUX
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#if PLATFORM_WINDOWS
using suseconds_t = long;
using socklen_t = int;
using buffer_t = char;
#elif PLATFORM_LINUX
using buffer_t = void;
#endif

udpsocket::udpsocket()
{
	m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket == -1) [[unlikely]]
	{
		LOG(Error, UdpSocket, "Failed to create socket. Error code: {0}", errno);
	}
}

udpsocket::~udpsocket()
{
	if (m_socket != -1) [[likely]]
#if PLATFORM_WINDOWS
		closesocket(m_socket);
#elif PLATFORM_LINUX
		::close(m_socket);
#endif
}

bool udpsocket::bind(int32_t port) const
{
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = ur::net::hton16(port);

	if (::bind(m_socket, (struct sockaddr*)&address, sizeof(address)) == -1)
	{
		LOG(Error, UdpSocket, "Failed to bind socket to port {0}", port);
		return false;
	}

	return true;
}

uint16_t udpsocket::getPort() const
{
	sockaddr_storage addr{};
	socklen_t socklen = sizeof(sockaddr_storage);
	const int res = getsockname(m_socket, (sockaddr*)&addr, &socklen) == 0;
	if (res == 0) [[unlikely]]
	{
		LOG(Error, UdpSocket, "Failed to get port. Error code: {0}", errno);
		return 0;
	}
	return ur::net::ntoh16(((sockaddr_in&)addr).sin_port);
}

socket_t udpsocket::getNativeSocket() const
{
	return m_socket;
}

int32_t udpsocket::sendTo(void* buffer, size_t bufferSize, const internetaddr* addr) const
{
	return ::sendto(m_socket, (const buffer_t*)buffer, bufferSize, 0, (struct sockaddr*)&addr->getAddr(), sizeof(sockaddr_in));
}

int32_t udpsocket::recvFrom(void* buffer, size_t bufferSize, internetaddr* addr) const
{
	socklen_t socklen = sizeof(sockaddr_in);
	return ::recvfrom(m_socket, (buffer_t*)buffer, bufferSize, 0, (struct sockaddr*)&addr->getAddr(), &socklen);
}

bool udpsocket::setReuseAddr(bool bAllowReuse) const
{
#if PLATFORM_WINDOWS
	const bool opt = bAllowReuse;
#elif PLATFORM_LINUX
	const int opt = bAllowReuse;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (buffer_t*)&opt, sizeof(opt)) == 0;
	return bOk;
}

bool udpsocket::setNonBlocking(bool bNonBlocking) const
{
#if PLATFORM_WINDOWS
	u_long value = bNonBlocking;
	return ioctlsocket(m_socket, FIONBIO, &value) == 0;
#elif PLATFORM_LINUX
	int flags = fcntl(m_socket, F_GETFL, 0);
	if (flags == -1) [[unlikely]]
		return false;
	flags = bNonBlocking ? flags | O_NONBLOCK : flags ^ (flags & O_NONBLOCK);
	return fcntl(m_socket, F_SETFL, flags) != -1;
#endif
}

bool udpsocket::setSendBufferSize(int32_t size, int32_t& newSize) const
{
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size)) == 0;
	socklen_t sizeSize = sizeof(size);
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&newSize, &sizeSize);
	return bOk;
}

bool udpsocket::setRecvBufferSize(int32_t size, int32_t& newSize) const
{
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (buffer_t*)&size, sizeof(size)) == 0;
	socklen_t sizeSize = sizeof(size);
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (buffer_t*)&newSize, &sizeSize);
	return bOk;
}

bool udpsocket::setRecvTimeoutUs(int64_t timeoutUs) const
{
#if PLATFORM_WINDOWS
	DWORD time = (timeoutUs + 999) / 1000;
#elif PLATFORM_LINUX
	timeval time;
	time.tv_sec = timeoutUs / 1000000;
	time.tv_usec = timeoutUs % 1000000;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (buffer_t*)&time, sizeof(time)) == 0;
	return bOk;
}

bool udpsocket::setSendTimeoutUs(int64_t timeoutUs) const
{
#if PLATFORM_WINDOWS
	DWORD time = (timeoutUs + 999) / 1000;
#elif PLATFORM_LINUX
	timeval time;
	time.tv_sec = timeoutUs / 1000000;
	time.tv_usec = timeoutUs % 1000000;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (buffer_t*)&time, sizeof(time)) == 0;
	return bOk;
}

bool udpsocket::waitForReadUs(int64_t timeoutUs) const
{
	timeval time;
	time.tv_sec = timeoutUs / 1000000;
	time.tv_usec = timeoutUs % 1000000;

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = ::select(static_cast<int>(m_socket + 1), &socketSet, NULL, NULL, &time);
	return selectRes > 0;
}

bool udpsocket::waitForWriteUs(int64_t timeoutUs) const
{
	timeval time;
	time.tv_sec = timeoutUs / 1000000;
	time.tv_usec = timeoutUs % 1000000;

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = ::select(static_cast<int>(m_socket + 1), NULL, &socketSet, NULL, &time);
	return selectRes > 0;
}

bool udpsocket::isValid() const
{
	return m_socket != -1;
}