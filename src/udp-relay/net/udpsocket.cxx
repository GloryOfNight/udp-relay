// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/net/udpsocket.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/socket_address.hxx"

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
using socklen_t = int;
using buffer_t = char;
const udpsocket::socket_t socketInvalid = INVALID_SOCKET;
#elif UR_PLATFORM_LINUX
using buffer_t = void;
const udpsocket::socket_t socketInvalid = -1;
#endif

udpsocket::udpsocket() noexcept
	: m_socket{socketInvalid}
{
}

udpsocket::udpsocket(udpsocket&& from) noexcept
{
	m_socket = from.m_socket;
	from.m_socket = socketInvalid;
}

udpsocket& udpsocket::operator=(udpsocket&& from) noexcept
{
	m_socket = from.m_socket;
	from.m_socket = socketInvalid;
	return *this;
}

udpsocket::~udpsocket() noexcept
{
	close();
}

udpsocket udpsocket::make(bool makeIpv6) noexcept
{
	udpsocket newSocket{};
	const int af = makeIpv6 ? AF_INET6 : AF_INET;
	newSocket.m_socket = ::socket(af, SOCK_DGRAM, 0);
	if (!newSocket.isValid()) [[unlikely]]
	{
		LOG(Error, UdpSocket, "Failed to create socket. Error code: {0}", errno);
		return udpsocket();
	}
	return newSocket;
}

void udpsocket::close() noexcept
{
	if (isValid())
	{
#if UR_PLATFORM_WINDOWS
		closesocket(m_socket);
#elif UR_PLATFORM_LINUX
		::close(m_socket);
#endif
		m_socket = socketInvalid;
	}
}

bool udpsocket::bind(const socket_address& addr) const
{
	sockaddr_storage saddr{};
	addr.copyToNative(saddr);

	if (::bind(m_socket, (sockaddr*)&saddr, sizeof(saddr)) == -1) [[unlikely]]
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

	socket_address addr{};
	addr.copyFromNative(saddr);
	return addr.getPort();
}

udpsocket::socket_t udpsocket::getNativeSocket() const noexcept
{
	return m_socket;
}

bool udpsocket::isValid() const noexcept
{
	return m_socket != socketInvalid;
}

bool udpsocket::isIpv6() const noexcept
{
	sockaddr_storage addr{};
	socklen_t len = sizeof(addr);

	if (getsockname(m_socket, (sockaddr*)&addr, &len) == -1) [[unlikely]]
		return false;

	return addr.ss_family == AF_INET6;
}

int32_t udpsocket::sendTo(void* buffer, size_t bufferSize, const socket_address& addr) const noexcept
{
	sockaddr_storage saddr{};
	const socklen_t slen = sizeof(saddr);

	addr.copyToNative(saddr);
	return ::sendto(m_socket, (const buffer_t*)buffer, bufferSize, 0, (struct sockaddr*)&saddr, slen);
}

int32_t udpsocket::recvFrom(void* buffer, size_t bufferSize, socket_address& addr) const noexcept
{
	sockaddr_storage saddr{};
	socklen_t slen = sizeof(saddr);

	const int32_t res = ::recvfrom(m_socket, (buffer_t*)buffer, bufferSize, 0, (struct sockaddr*)&saddr, &slen);
	addr.copyFromNative(saddr);
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

bool udpsocket::setRecvTimeoutUs(std::chrono::microseconds timeout) const noexcept
{
#if UR_PLATFORM_WINDOWS
	DWORD time = (timeout.count() + 999) / 1000;
#elif UR_PLATFORM_LINUX
	timeval time;
	time.tv_sec = timeout.count() / 1000000;
	time.tv_usec = timeout.count() % 1000000;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (buffer_t*)&time, sizeof(time)) == 0;
	return bOk;
}

bool udpsocket::setSendTimeoutUs(std::chrono::microseconds timeout) const noexcept
{
#if UR_PLATFORM_WINDOWS
	DWORD time = (timeout.count() + 999) / 1000;
#elif UR_PLATFORM_LINUX
	timeval time;
	time.tv_sec = timeout.count() / 1000000;
	time.tv_usec = timeout.count() % 1000000;
#endif
	const bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (buffer_t*)&time, sizeof(time)) == 0;
	return bOk;
}

bool udpsocket::waitForRead(std::chrono::microseconds timeout) const noexcept
{
	timeval time;
	time.tv_sec = timeout.count() / 1000000;
	time.tv_usec = timeout.count() % 1000000;

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = ::select(static_cast<int>(m_socket + 1), &socketSet, NULL, NULL, &time);
	return selectRes > 0;
}

bool udpsocket::waitForWrite(std::chrono::microseconds timeout) const noexcept
{
	timeval time;
	time.tv_sec = timeout.count() / 1000000;
	time.tv_usec = timeout.count() % 1000000;

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = ::select(static_cast<int>(m_socket + 1), NULL, &socketSet, NULL, &time);
	return selectRes > 0;
}