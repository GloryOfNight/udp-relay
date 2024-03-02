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

	int32_t bufferSize{};
	m_socket->setSendBufferSize(0x20000, bufferSize);
	m_socket->setRecvBufferSize(0x20000, bufferSize);

	return true;
}

bool relay::run()
{
	if (!init())
		return false;

	LOG(Display, "Relay initialized and running on port {0}", mainPort);

	std::unique_ptr<internetaddr> recvAddr = udpsocketFactory::createInternetAddrUnique();

	std::array<uint8_t, 1024> buffer{};
	m_running = true;
	while (m_running)
	{
		auto bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), recvAddr.get());
		if (bytesRead > 0)
		{
			//const auto findRes = m_addressMappedChannels.find(std::shared_ptr<internetaddr>(recvAddr.get()));
			//// if has a channel mapped to the address aswell as two peers, relay
			//if (findRes != m_addressMappedChannels.end() && findRes->second.m_peerA && findRes->second.m_peerB)
			//{
			//	const auto sendAddr = *findRes->second.m_peerA != *recvAddr ? findRes->second.m_peerA : findRes->second.m_peerB;

			//	LOG(Verbose, "Relaying packet of {0} bytes from {1} to {2}", bytesRead, sendAddr->toString(), recvAddr->toString());
			//	auto bytesSent = m_socket->sendTo(buffer.data(), bytesRead, sendAddr.get());

			//	continue;
			//}

			if (bytesRead == 1024)
			{
				const handshake_header* header = reinterpret_cast<const handshake_header*>(buffer.data());

				LOG(Verbose, "Received header: type: {0}, length: {1} from {2}", header->type, header->length, recvAddr->toString());

				auto nthType = NETWORK_TO_HOST_16(header->type);
				auto nthLength = NETWORK_TO_HOST_16(header->length);

				LOG(Verbose, "Swapped header: type: {0}, length: {1} from {2}", nthType, nthLength, recvAddr->toString());

				if (nthType == 0x01 && nthLength == 1024)
				{
					LOG(Display, "you win!");
				}
			}
		}
	}

	return true;
}

void relay::stop()
{
	m_running = false;
}