#include "udpsocketWin.hxx"

#include "internetaddrWin.hxx"
#include "log.hxx"

#include <WinSock2.h>

udpsocketWin::udpsocketWin()
{
	m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket == INVALID_SOCKET) [[unlikely]]
	{
		LOG(Error, "Failed to create socket. Error code: {0}", WSAGetLastError());
	}
}

udpsocketWin::~udpsocketWin()
{
	if (m_socket != INVALID_SOCKET) [[likely]]
		closesocket(m_socket);
}

bool udpsocketWin::bind(int32_t port)
{
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (::bind(m_socket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		LOG(Error, "Failed to bind socket to port {0} with error {1}", port, WSAGetLastError());
		return -1;
	}

	return true;
}

int32_t udpsocketWin::sendTo(void* buffer, size_t bufferSize, const internetaddr* addr)
{
	return ::sendto(m_socket, (const char*)buffer, bufferSize, 0, (struct sockaddr*)&addr->getAddr(), sizeof(sockaddr_in));
}

int32_t udpsocketWin::recvFrom(void* buffer, size_t bufferSize, internetaddr* addr)
{
	int socklen = sizeof(sockaddr_in);
	return ::recvfrom(m_socket, (char*)buffer, bufferSize, 0, (struct sockaddr*)&addr->getAddr(), &socklen);
}

bool udpsocketWin::setNonBlocking(bool bNonBlocking)
{
	u_long Value = bNonBlocking ? true : false;
	return ioctlsocket(m_socket, FIONBIO, &Value) == 0;
}

bool udpsocketWin::setSendBufferSize(int32_t size, int32_t& newSize)
{
	int sizeSize = sizeof(size);
	bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int32_t)) == 0;
	getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&newSize, &sizeSize);
	return bOk;
}

bool udpsocketWin::setRecvBufferSize(int32_t size, int32_t& newSize)
{
	int sizeSize = sizeof(size);
	bool bOk = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(int32_t)) == 0;
	getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&newSize, &sizeSize);
	return bOk;
}

bool udpsocketWin::waitForRead(int32_t timeoutms)
{
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = timeoutms * 1000;

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = select(static_cast<int>(m_socket + 1), &socketSet, NULL, NULL, &time);
	return selectRes > 0;
}

bool udpsocketWin::waitForWrite(int32_t timeoutms)
{
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = timeoutms * 1000;

	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(m_socket, &socketSet);

	const auto selectRes = select(static_cast<int>(m_socket + 1), NULL, &socketSet, NULL, &time);
	return selectRes > 0;
}

bool udpsocketWin::isValid()
{
	return m_socket > 0;
}
