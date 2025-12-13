// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/net/udpsocket.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/net/internetaddr.hxx"
#include "udp-relay/net/network_utils.hxx"

#if UR_PLATFORM_WINDOWS
#include <WinSock2.h>
#include <ws2ipdef.h>
#elif UR_PLATFORM_LINUX
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#if UR_PLATFORM_WINDOWS
using suseconds_t = long;
using socklen_t = int;
using buffer_t = char;
const udpsocket::socket_t socketInvalid = INVALID_SOCKET;
#elif UR_PLATFORM_LINUX
using buffer_t = void;
const udpsocket::socket_t socketInvalid = -1;
#endif

udpsocket::udpsocket(bool ipv6) noexcept
{
	const int af = ipv6 ? AF_INET6 : AF_INET;
	m_socket = ::socket(af, SOCK_DGRAM, 0);
	if (!isValid()) [[unlikely]]
	{
		LOG(Error, UdpSocket, "Failed to create socket. Error code: {0}", errno);
	}
}

udpsocket::~udpsocket() noexcept
{
	if (isValid())
#if UR_PLATFORM_WINDOWS
		closesocket(m_socket);
#elif UR_PLATFORM_LINUX
		::close(m_socket);
#endif
}

bool udpsocket::isIpv6() const noexcept
{
	sockaddr_storage addr{};
	socklen_t len = sizeof(addr);

	if (getsockname(m_socket, (sockaddr*)&addr, &len) == -1)
		return false;

	return addr.ss_family == AF_INET6;
}

bool udpsocket::bind(const internetaddr& addr) const
{
	sockaddr_storage saddr{};
	addr.copyToNative(saddr);

	if (::bind(m_socket, (sockaddr*)&saddr, sizeof(saddr)) == -1)
	{
		return false;
	}
	return true;
}

uint16_t udpsocket::getPort() const
{
	sockaddr_storage saddr{};
	socklen_t slen = sizeof(saddr);

	const int res = getsockname(m_socket, (sockaddr*)&saddr, &slen) == 0;
	if (res == 0) [[unlikely]]
	{
		LOG(Error, UdpSocket, "Failed to get port. Error code: {0}", errno);
		return 0;
	}

	internetaddr addr{};
	addr.copyFromNative(saddr);
	return addr.getPort();
}

udpsocket::socket_t udpsocket::getNativeSocket() const noexcept
{
	return m_socket;
}

int32_t udpsocket::sendTo(void* buffer, size_t bufferSize, const internetaddr* addr) const noexcept
{
	sockaddr_storage saddr{};
	const socklen_t slen = sizeof(saddr);

	addr->copyToNative(saddr);
	return ::sendto(m_socket, (const buffer_t*)buffer, bufferSize, 0, (struct sockaddr*)&saddr, slen);
}

int32_t udpsocket::recvFrom(void* buffer, size_t bufferSize, internetaddr* addr) const noexcept
{
	sockaddr_storage saddr{};
	socklen_t slen = sizeof(saddr);

	const int32_t res = ::recvfrom(m_socket, (buffer_t*)buffer, bufferSize, 0, (struct sockaddr*)&saddr, &slen);
	addr->copyFromNative(saddr);
	return res;
}

bool udpsocket::setOnlyIpv6(bool value) const noexcept
{
#if UR_PLATFORM_WINDOWS
	const bool opt = value;
#elif UR_PLATFORM_LINUX
	const int opt = value ? 1 : 0;
#endif
	return setsockopt(m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (const buffer_t*)&opt, sizeof(opt)) == 0;
}

bool udpsocket::setReuseAddr(bool bAllowReuse) const noexcept
{
#if UR_PLATFORM_WINDOWS
	const bool opt = bAllowReuse;
#elif UR_PLATFORM_LINUX
	const int opt = bAllowReuse;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const buffer_t*)&opt, sizeof(opt)) == 0;
	return bOk;
}

bool udpsocket::setNonBlocking(bool bNonBlocking) const noexcept
{
#if UR_PLATFORM_WINDOWS
	u_long value = bNonBlocking;
	return ioctlsocket(m_socket, FIONBIO, &value) == 0;
#elif UR_PLATFORM_LINUX
	int flags = fcntl(m_socket, F_GETFL, 0);
	if (flags == -1) [[unlikely]]
		return false;
	flags = bNonBlocking ? flags | O_NONBLOCK : flags ^ (flags & O_NONBLOCK);
	return fcntl(m_socket, F_SETFL, flags) != -1;
#endif
}

bool udpsocket::setSendBufferSize(int32_t size) const noexcept
{
	return setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size)) == 0;
}

int32_t udpsocket::getSendBufferSize() const noexcept
{
	int32_t size{};
	socklen_t sizeSize = sizeof(size);
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (buffer_t*)&size, &sizeSize);
	return size;
}

bool udpsocket::setRecvBufferSize(int32_t size) const noexcept
{
	return setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (buffer_t*)&size, sizeof(size)) == 0;
}

int32_t udpsocket::getRecvBufferSize() const noexcept
{
	int32_t size{};
	socklen_t sizeSize = sizeof(size);
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (buffer_t*)&size, &sizeSize);
	return size;
}

bool udpsocket::setRecvTimeoutUs(int64_t timeoutUs) const noexcept
{
#if UR_PLATFORM_WINDOWS
	DWORD time = (timeoutUs + 999) / 1000;
#elif UR_PLATFORM_LINUX
	timeval time;
	time.tv_sec = timeoutUs / 1000000;
	time.tv_usec = timeoutUs % 1000000;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (buffer_t*)&time, sizeof(time)) == 0;
	return bOk;
}

bool udpsocket::setSendTimeoutUs(int64_t timeoutUs) const noexcept
{
#if UR_PLATFORM_WINDOWS
	DWORD time = (timeoutUs + 999) / 1000;
#elif UR_PLATFORM_LINUX
	timeval time;
	time.tv_sec = timeoutUs / 1000000;
	time.tv_usec = timeoutUs % 1000000;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (buffer_t*)&time, sizeof(time)) == 0;
	return bOk;
}

bool udpsocket::waitForReadUs(int64_t timeoutUs) const noexcept
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

bool udpsocket::waitForWriteUs(int64_t timeoutUs) const noexcept
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

bool udpsocket::isValid() const noexcept
{
	return m_socket != socketInvalid;
}