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

	m_socket->bind(mainPort);
	m_socket->setNonBlocking(true);

	int32_t bufferSize{};
	m_socket->setSendBufferSize(0x20000, bufferSize);
	m_socket->setRecvBufferSize(0x20000, bufferSize);

	return true;
}

channel& relay::createChannel(const guid& inGuid)
{
	channel newChannel{};
	newChannel.m_guid = inGuid;

	LOG(Display, "Created channel with guid: {0}", inGuid.toString());

	return *m_channels.emplace_after(m_channels.before_begin(), newChannel);
}

bool relay::run()
{
	if (!init())
		return false;

	LOG(Display, "Relay initialized and running on port {0}", mainPort);

	std::shared_ptr<internetaddr> recvAddr = udpsocketFactory::createInternetAddrUnique();

	std::array<uint8_t, 1024> buffer{};
	m_running = true;
	while (m_running)
	{
		auto bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), recvAddr.get());
		if (bytesRead > 0)
		{
			const auto findRes = m_addressMappedChannels.find(recvAddr);
			// if has a channel mapped to the address aswell as two peers, relay
			if (findRes != m_addressMappedChannels.end() && findRes->second.m_peerA && findRes->second.m_peerB)
			{
				const auto sendAddr = *findRes->second.m_peerA != *recvAddr ? findRes->second.m_peerA : findRes->second.m_peerB;

				LOG(Verbose, "Relaying packet of {0} bytes from {1} to {2}", bytesRead, sendAddr->toString(), recvAddr->toString());
				auto bytesSent = m_socket->sendTo(buffer.data(), bytesRead, sendAddr.get());

				continue;
			}

			if (bytesRead == 1024) // const handshake packet size
			{
				const handshake_header* header = reinterpret_cast<const handshake_header*>(buffer.data());

				LOG(Verbose, "Received header: type: {0}, length: {1} from {2}", header->m_type, header->m_length, recvAddr->toString());

				auto nthType = NETWORK_TO_HOST_16(header->m_type);
				auto nthLength = NETWORK_TO_HOST_16(header->m_length);

				LOG(Verbose, "Swapped header: type: {0}, length: {1} from {2}", nthType, nthLength, recvAddr->toString());

				if (nthType == 0x01 && nthLength == 992) // const handshake packet values
				{
					const auto& recvGuid = header->m_id;
					const guid htnGuid = guid(NETWORK_TO_HOST_16(recvGuid.m_a), NETWORK_TO_HOST_16(recvGuid.m_b), NETWORK_TO_HOST_16(recvGuid.m_c), NETWORK_TO_HOST_16(recvGuid.m_d));

					auto guidChannel = m_guidMappedChannels.find(htnGuid);
					if (guidChannel == m_guidMappedChannels.end())
					{
						channel& newChannel = createChannel(htnGuid);
						newChannel.m_peerA = recvAddr;
					}
					else if (*guidChannel->second.m_peerA != *recvAddr && guidChannel->second.m_peerB == nullptr)
					{
						guidChannel->second.m_peerB = recvAddr;

						LOG(Display, "Creating relay for session: {0} with peers: {1}, {2}.", htnGuid.toString(), guidChannel->second.m_peerA->toString(), guidChannel->second.m_peerB->toString());

						m_addressMappedChannels.emplace(guidChannel->second.m_peerA, guidChannel->second);
						m_addressMappedChannels.emplace(guidChannel->second.m_peerB, guidChannel->second);
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