#include "relay.hxx"

#include "socket/udpsocketFactory.hxx"

#include "arguments.hxx"
#include "log.hxx"

#include <array>

bool relay::init()
{
	m_socket = udpsocketFactory::createUdpSocket();
	if (!m_socket->isValid())
	{
		LOG(Error, "Failed to create socket!");
		return false;
	}

	m_socket->bind(mainPort);
	m_socket->setNonBlocking(true);

	return true;
}

bool relay::run()
{
	if (!init())
		return false;

	LOG(Display, "Relay initialized and running on port {0}", mainPort);

	std::unique_ptr<internetaddr> recvAddr = udpsocketFactory::createInternetAddrUnique();

	std::array<uint8_t, 1024> buffer{};
	bRunning = true;
	while (bRunning)
	{
		auto bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), recvAddr.get());
		if (bytesRead > 0)
		{
			LOG(Display, "Received {0} bytes from {1}", bytesRead, recvAddr->toString());
			auto bytesSent = m_socket->sendTo(buffer.data(), bytesRead, recvAddr.get());
			if (bytesSent > 0)
			{
				LOG(Display, "Sent {0} bytes to {1}", bytesSent, recvAddr->toString());
			}
			else
			{
				LOG(Error, "Failed to send data to socket!");
			}
		}
	}

	return true;
}

void relay::stop()
{
	bRunning = false;
}