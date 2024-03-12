#include "relay.hxx"

#include "socket/udpsocketFactory.hxx"

#include "log.hxx"

#include <array>

bool relay::run(const uint16_t port)
{
	if (!init(port))
		return false;

	LOG(Display, "Relay initialized, port {0}", port);

	std::array<uint8_t, 1024> buffer{};

	m_lastCleanupTime = std::chrono::steady_clock::now();
	m_running = true;
	while (m_running)
	{
		conditionalCleanup(false);
		if (!m_socket->waitForRead(5))
			continue;

		const auto m_tickStartTime = std::chrono::steady_clock::now();

		std::shared_ptr<internetaddr> recvAddr = udpsocketFactory::createInternetAddr();
		const int32_t bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), recvAddr.get());
		if (bytesRead > 0)
		{
			const auto findRes = m_addressMappedChannels.find(recvAddr);
			// if recvAddr has a channel mapped to it, as well as two valid peers, relay packet to the other peer
			if (findRes != m_addressMappedChannels.end() && findRes->second.m_peerA && findRes->second.m_peerB)
			{
				auto& currentChannel = findRes->second;

				currentChannel.m_lastUpdated = m_tickStartTime;

				currentChannel.m_stats.m_packetsReceived++;
				currentChannel.m_stats.m_bytesReceived += bytesRead;

				const auto& sendAddr = *findRes->second.m_peerA != *recvAddr ? findRes->second.m_peerA : findRes->second.m_peerB;
				int32_t bytesSent = m_socket->sendTo(buffer.data(), bytesRead, sendAddr.get());
				if (bytesSent == -1 && errno == SE_WOULDBLOCK)
				{
					if (m_socket->waitForWrite(5))
						bytesSent = m_socket->sendTo(buffer.data(), bytesRead, sendAddr.get());
				}

				if (bytesSent > 0)
				{
					currentChannel.m_stats.m_packetsSent++;
					currentChannel.m_stats.m_bytesSent += bytesSent;
				}
			}
			else if (checkHandshakePacket(buffer, bytesRead))
			{
				const auto& recvGuid = reinterpret_cast<const handshake_header*>(buffer.data())->m_guid;
				const guid nthGuid = guid(NETWORK_TO_HOST_32(recvGuid.m_a), NETWORK_TO_HOST_32(recvGuid.m_b), NETWORK_TO_HOST_32(recvGuid.m_c), NETWORK_TO_HOST_32(recvGuid.m_d));

				auto guidChannel = m_guidMappedChannels.find(nthGuid);
				if (guidChannel == m_guidMappedChannels.end())
				{
					channel& newChannel = createChannel(nthGuid);
					newChannel.m_peerA = std::move(recvAddr);
					newChannel.m_lastUpdated = m_tickStartTime;

					m_guidMappedChannels.emplace(newChannel.m_guid, newChannel);

					LOG(Display, "Created channel for guid: \"{0}\"", newChannel.m_guid.toString());
				}
				else if (*guidChannel->second.m_peerA != *recvAddr && guidChannel->second.m_peerB == nullptr)
				{
					guidChannel->second.m_peerB = std::move(recvAddr);
					guidChannel->second.m_lastUpdated = m_tickStartTime;

					m_addressMappedChannels.emplace(guidChannel->second.m_peerA, guidChannel->second);
					m_addressMappedChannels.emplace(guidChannel->second.m_peerB, guidChannel->second);

					LOG(Display, "Channel relay established for session: \"{0}\" with peers: {1}, {2}.", nthGuid.toString(), guidChannel->second.m_peerA->toString(), guidChannel->second.m_peerB->toString());
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

bool relay::init(const uint16_t port)
{
	m_socket = udpsocketFactory::createUdpSocket();
	if (m_socket == nullptr || !m_socket->isValid())
	{
		LOG(Error, "Failed to create socket!");
		return false;
	}

	if (!m_socket->bind(port))
	{
		LOG(Error, "Failed bind to {0} port", port);
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
	return m_channels.emplace_back(newChannel);
}

bool relay::conditionalCleanup(bool force)
{
	const auto now = std::chrono::steady_clock::now();
	const auto secondsSinceLastCleanup = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastCleanupTime).count();
	if (secondsSinceLastCleanup > 60 || force)
	{
		for (auto it = m_channels.begin(); it != m_channels.end();)
		{
			const auto inactiveSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - it->m_lastUpdated).count();
			if (inactiveSeconds > 30)
			{
				const auto channelGuidStr = it->m_guid.toString();
				const auto stats = it->m_stats;
				LOG(Display, "Channel \"{0}\" has been inactive for {1} seconds, removing.", channelGuidStr, inactiveSeconds);
				LOG(Display, "Channel \"{0}\" packet amount and total bytes. Relayed: {1} ({2}); Dropped: {3} ({4})", channelGuidStr, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent);

				m_addressMappedChannels.erase(it->m_peerA);
				m_addressMappedChannels.erase(it->m_peerB);
				m_guidMappedChannels.erase(it->m_guid);

				it = m_channels.erase(it);
			}
			else
			{
				++it;
			}
		}

		m_lastCleanupTime = now;
		return true;
	}

	return false;
}
