#include "udp-relay/unix/udpsocketUnix.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/unix/internetaddrUnix.hxx"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

udpsocketUnix::udpsocketUnix()
{
	m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket == -1) [[unlikely]]
	{
		LOG(Error, "Failed to create socket. Error code: {0}", errno);
	}
}

bool udpsocketUnix::bind(int32_t port)
{
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (::bind(m_socket, (struct sockaddr*)&address, sizeof(address)) == -1)
	{
		LOG(Error, "Failed to bind socket to port {0}", port, ".");
		return -1;
	}

	return true;
}

udpsocketUnix::~udpsocketUnix()
{
	if (m_socket != -1) [[likely]]
		::close(m_socket);
}

int32_t udpsocketUnix::sendTo(void* buffer, size_t bufferSize, const internetaddr* addr)
{
	return ::sendto(m_socket, buffer, bufferSize, 0, (struct sockaddr*)&addr->getAddr(), sizeof(sockaddr_in));
}

int32_t udpsocketUnix::recvFrom(void* buffer, size_t bufferSize, internetaddr* addr)
{
	socklen_t socklen = sizeof(sockaddr_in);
	return ::recvfrom(m_socket, buffer, bufferSize, 0, (struct sockaddr*)&addr->getAddr(), &socklen);
}

uint16_t udpsocketUnix::getPort() const
{
	sockaddr_storage addr{};
	socklen_t socklen = sizeof(sockaddr_storage);
	const int res = getsockname(m_socket, (sockaddr*)&addr, &socklen) == 0;
	if (res == 0) [[unlikely]]
	{
		LOG(Error, "Failed to get port. Error code: {0}", errno);
		return 0;
	}
	return ntohs(((sockaddr_in&)addr).sin_port);
}

bool udpsocketUnix::setNonBlocking(bool bNonBlocking)
{
	int flags = fcntl(m_socket, F_GETFL, 0);
	if (flags == -1) [[unlikely]]
		return false;
	flags = bNonBlocking ? flags | O_NONBLOCK : flags ^ (flags & O_NONBLOCK);
	return fcntl(m_socket, F_SETFL, flags) != -1;
}

bool udpsocketUnix::setSendBufferSize(int32_t size, int32_t& newSize)
{
	socklen_t sizeSize = sizeof(size);
	bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int32_t)) == 0;
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&newSize, &sizeSize);
	return bOk;
}

bool udpsocketUnix::setRecvBufferSize(int32_t size, int32_t& newSize)
{
	socklen_t sizeSize = sizeof(size);
	bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int32_t)) == 0;
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&newSize, &sizeSize);
	return bOk;
}

bool udpsocketUnix::waitForRead(int32_t timeoutUs)
{
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = static_cast<suseconds_t>(timeoutUs);

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	timeval* timePtr = timeoutUs == 0 ? nullptr : &time;

	const auto selectRes = ::select(static_cast<int>(m_socket + 1), &socketSet, NULL, NULL, &time);
	return selectRes > 0;
}

bool udpsocketUnix::waitForWrite(int32_t timeoutUs)
{
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = static_cast<suseconds_t>(timeoutUs);

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = ::select(static_cast<int>(m_socket + 1), NULL, &socketSet, NULL, &time);
	return selectRes > 0;
}

bool udpsocketUnix::isValid() const
{
	return m_socket != -1;
}