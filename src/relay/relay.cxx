#include "relay.hxx"

#include "socket/udpsocketFactory.hxx"

#include "arguments.hxx"
#include "log.hxx"

#include <array>

bool relay::init()
{
	m_socket = udpsocketFactory::createUdpSocket();
	if (m_socket == nullptr || !m_socket->isValid())
	{
		LOG(Error, "Failed to create socket!");
		return false;
	}

	if (!m_socket->bind(args::port))
	{
		LOG(Error, "Failed bind to {0} port", args::port);
		return false;
	}

	if (!m_socket->setNonBlocking(true))
	{
		LOG(Error, "Failed set socket to non-blocking mode");
		return false;
	}

	const int32_t wantedBufferSize = 0x20000; // same value as in unreal server
	int32_t actualBufferSize{};

	if (!m_socket->setSendBufferSize(wantedBufferSize, actualBufferSize))
		LOG(Error, "Error with setting send buffer size");
	LOG(Verbose, "Send buffer size, wanted: {0}, actual: {1}", wantedBufferSize, actualBufferSize);


	if (!m_socket->setRecvBufferSize(wantedBufferSize, actualBufferSize))
		LOG(Error, "Error with setting recv buffer size");
	LOG(Verbose, "Receive buffer size, wanted: {0}, actual: {1}", wantedBufferSize, actualBufferSize);

	return true;
}

channel& relay::createChannel(const guid& inGuid)
{
	channel newChannel{};
	newChannel.m_guid = inGuid;
	return *m_channels.emplace_after(m_channels.before_begin(), newChannel);
}

bool relay::run()
{
	if (!init())
		return false;

	LOG(Display, "Relay initialized and running on port {0}", args::port);

	std::array<uint8_t, 1024> buffer{};
	m_running = true;
	while (m_running)
	{
		if (!m_socket->waitForRead(1000))
			continue;

		std::shared_ptr<internetaddr> recvAddr = udpsocketFactory::createInternetAddr();
		const int32_t bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), recvAddr.get());
		if (bytesRead > 0)
		{
			const auto findRes = m_addressMappedChannels.find(recvAddr);
			// if recvAddr has a channel mapped to it, as well as two valid peers, relay packet to the other peer
			if (findRes != m_addressMappedChannels.end() && findRes->second.m_peerA && findRes->second.m_peerB)
			{
				const auto& sendAddr = *findRes->second.m_peerA != *recvAddr ? findRes->second.m_peerA : findRes->second.m_peerB;
				int32_t bytesSent = m_socket->sendTo(buffer.data(), bytesRead, sendAddr.get());

				if (bytesSent == -1)
				{
					if (errno == SE_WOULDBLOCK)
					{
						if (m_socket->waitForWrite(5))
						{
							m_socket->sendTo(buffer.data(), bytesRead, sendAddr.get());
						}
						else 
						{
							LOG(Error, "Packet dropped, socket not ready for write {0}.", sendAddr->toString());
						}
					}
					else 
					{
						LOG(Error, "Failed to send data to {0}. Error code: {1}", sendAddr->toString(), errno);
					}
				}

				continue;
			}
			else if (bytesRead == 1024) // const handshake packet size
			{
				const handshake_header* header = reinterpret_cast<const handshake_header*>(buffer.data());

				auto nthType = NETWORK_TO_HOST_16(header->m_type);
				auto nthLength = NETWORK_TO_HOST_16(header->m_length);

				if (nthType == 0x01 && nthLength == 992) // const handshake packet values
				{
					const auto& recvGuid = header->m_id;
					const guid htnGuid = guid(NETWORK_TO_HOST_32(recvGuid.m_a), NETWORK_TO_HOST_32(recvGuid.m_b), NETWORK_TO_HOST_32(recvGuid.m_c), NETWORK_TO_HOST_32(recvGuid.m_d));

					auto guidChannel = m_guidMappedChannels.find(htnGuid);
					if (guidChannel == m_guidMappedChannels.end())
					{
						channel& newChannel = createChannel(htnGuid);
						newChannel.m_peerA = std::move(recvAddr);

						m_guidMappedChannels.emplace(newChannel.m_guid, newChannel);

						LOG(Display, "Created channel for guid: {0}", newChannel.m_guid.toString());
					}
					else if (*guidChannel->second.m_peerA != *recvAddr && guidChannel->second.m_peerB == nullptr)
					{
						guidChannel->second.m_peerB = std::move(recvAddr);

						m_addressMappedChannels.emplace(guidChannel->second.m_peerA, guidChannel->second);
						m_addressMappedChannels.emplace(guidChannel->second.m_peerB, guidChannel->second);

						LOG(Display, "Channel relay established for session: {0} with peers: {1}, {2}.", htnGuid.toString(), guidChannel->second.m_peerA->toString(), guidChannel->second.m_peerB->toString());
					}
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