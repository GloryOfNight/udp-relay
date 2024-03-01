#include "relay.hxx"

#include "arguments.hxx"
#include "log.hxx"

#include <array>

// clang-format off sockets
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
// clang-format on

static int createSocket(int32_t port)
{
	LOG(Display, "Creating socket on port {0}", port);

	int new_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (new_socket == -1)
	{
		LOG(Error, "Failed to create socket.");
		return -1;
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(new_socket, (struct sockaddr*)&address, sizeof(address)) == -1)
	{
		LOG(Error, "Failed to bind socket to port {0}", port, ".");
		return -1;
	}

	// Make the socket non-blocking
	int flags = fcntl(new_socket, F_GETFL, 0);
	if (flags == -1)
	{
		LOG(Error, "Failed to get socket flags.");
		return -1;
	}

	if (fcntl(new_socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		LOG(Error, "Failed to set socket to non-blocking mode.");
		return -1;
	}
	return new_socket;
}

bool relay::init()
{
	m_socket = createSocket(mainPort);
	if (m_socket == -1)
	{
		LOG(Error, "Failed to create socket.");
		return false;
	}

	return true;
}

void relay::run()
{
	std::array<uint8_t, 1024> buffer{};
	bRunning = true;
	while (bRunning)
	{
		sockaddr_in address{};
		socklen_t addressLength = sizeof(address);
		const ssize_t bytesRead = recvfrom(m_socket, buffer.data(), buffer.size(), 0, (struct sockaddr*)&address, &addressLength);
		if (bytesRead > 0)
		{
            LOG(Display, "Received message from {0}:{1} - {2} bytes", inet_ntoa(address.sin_addr), ntohs(address.sin_port), bytesRead);
            buffer = {};
		}
        else if (bytesRead == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                continue;
            }
            else 
            {
                LOG(Error, "Failed to receive message. {0}", errno);
            }
        }
	}
}

void relay::stop()
{
	bRunning = false;
}