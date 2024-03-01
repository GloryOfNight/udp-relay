#include "udpsocketUnix.hxx"

#include "internetaddrUnix.hxx"
#include "log.hxx"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
	const auto unixAddr = dynamic_cast<const internetaddrUnix*>(addr);
	if (unixAddr == nullptr) [[unlikely]]
		return -1;

	return ::sendto(m_socket, buffer, bufferSize, 0, (struct sockaddr*)&unixAddr->getAddr(), sizeof(sockaddr_in));
}

int32_t udpsocketUnix::recvFrom(void* buffer, size_t bufferSize, internetaddr* addr)
{
	auto unixAddr = dynamic_cast<internetaddrUnix*>(addr);
	if (unixAddr == nullptr) [[unlikely]]
		return -1;

    socklen_t socklen = sizeof(sockaddr_in);
    return ::recvfrom(m_socket, buffer, bufferSize, 0, (struct sockaddr*)&unixAddr->getAddr(), &socklen);
}

bool udpsocketUnix::setNonBlocking(bool value)
{
	int flags = fcntl(m_socket, F_GETFL, 0);
	if (flags == -1) [[unlikely]]
		return false;

	return fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) != -1;
}

bool udpsocketUnix::isValid()
{
	return m_socket != -1;
}