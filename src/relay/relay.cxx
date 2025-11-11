// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/udpsocket.hxx"

#include <thread>

bool relay::init(const relay_params& params)
{
	LOG(Verbose, Relay, "Begin initialization");

	uniqueUdpsocket newSocket = udpsocketFactory::createUdpSocket();
	if (newSocket == nullptr || !newSocket->isValid())
	{
		LOG(Error, Relay, "Failed to create socket!");
		return false;
	}

	LOG(Verbose, Relay, "Created primary udp socket");

	if (!newSocket->bind(params.m_primaryPort))
	{
		LOG(Error, Relay, "Failed bind to {0} port", params.m_primaryPort);
		return false;
	}

	if (!newSocket->setNonBlocking(true))
	{
		LOG(Error, Relay, "Failed set socket to non-blocking mode");
		return false;
	}

	LOG(Info, Relay, "Relay initialized, requested {0} port, actual: {1}", params.m_primaryPort, newSocket->getPort());

	int32_t actualBufferSize{};

	if (!newSocket->setSendBufferSize(params.m_socketSendBufferSize, actualBufferSize))
		LOG(Warning, Relay, "Failed set send buffer size to {}", params.m_socketSendBufferSize);
	LOG(Info, Relay, "Socket set send buffer size, requested: {0}, actual: {1}", params.m_socketSendBufferSize, actualBufferSize);

	if (!newSocket->setRecvBufferSize(params.m_socketRecvBufferSize, actualBufferSize))
		LOG(Warning, Relay, "Failed set recv buffer size to {}", params.m_socketRecvBufferSize);
	LOG(Info, Relay, "Socket set recv buffer size, requested: {0}, actual: {1}", params.m_socketRecvBufferSize, actualBufferSize);

	m_params = params;
	m_socket = std::move(newSocket);
	m_recvBuffer.resize(m_params.m_recvBufferSize);

	return true;
}

void relay::run()
{
	if (!m_socket)
	{
		LOG(Warning, Relay, "Cannot run while not initialized");
		return;
	}

	m_lastTickTime = std::chrono::steady_clock::now();
	m_lastCleanupTime = m_lastTickTime;
	m_running = true;

	while (m_running)
	{
		m_lastTickTime = std::chrono::steady_clock::now();

		if (m_socket->waitForReadUs(500))
			processIncoming();

		if (m_socket->waitForWriteUs(500))
			processOutcoming();

		conditionalCleanup();
	}
}

void relay::stop()
{
	m_running = false;
}

void relay::processIncoming()
{
	const int32_t bytesRead = m_socket->recvFrom(m_recvBuffer.data(), m_recvBuffer.size(), &m_recvAddr);
	if (bytesRead > 0)
	{
		const auto findRes = m_addressMappedChannels.find(m_recvAddr);
		// if recvAddr has a channel mapped to it, as well as two valid peers, relay packet to the other peer
		if (findRes != m_addressMappedChannels.end() && findRes->second.m_peerA.isValid() && findRes->second.m_peerB.isValid())
		{
			auto& currentChannel = findRes->second;

			currentChannel.m_lastUpdated = m_lastTickTime;

			currentChannel.m_stats.m_packetsReceived++;
			currentChannel.m_stats.m_bytesReceived += bytesRead;

			const auto& sendAddr = findRes->second.m_peerA != m_recvAddr ? findRes->second.m_peerA : findRes->second.m_peerB;

			// try relay packet immediately and if failed, add to send queue
			const auto bytesSend = m_socket->sendTo(m_recvBuffer.data(), bytesRead, &sendAddr);
			if (bytesSend > 0)
			{
				currentChannel.m_stats.m_packetsSent++;
				currentChannel.m_stats.m_bytesSent += bytesSend;
			}
			else
			{
				auto pendingPacket = pending_packet();
				pendingPacket.m_channelGuid = currentChannel.m_guid;
				pendingPacket.m_target = sendAddr;
				pendingPacket.m_buffer.resize(bytesRead);
				memcpy(pendingPacket.m_buffer.data(), m_recvBuffer.data(), bytesRead);
				m_sendQueue.push(std::move(pendingPacket));
			}
		}
		else if (checkHandshakePacket(m_recvBuffer.data(), bytesRead))
		{
			const auto& recvGuid = reinterpret_cast<const handshake_header*>(m_recvBuffer.data())->m_guid;

			const guid nthGuid = guid(ur::net::ntoh32(recvGuid.m_a), ur::net::ntoh32(recvGuid.m_b), ur::net::ntoh32(recvGuid.m_c), ur::net::ntoh32(recvGuid.m_d));

			auto guidChannel = m_guidMappedChannels.find(nthGuid);
			if (guidChannel == m_guidMappedChannels.end())
			{
				channel& newChannel = createChannel(nthGuid);
				newChannel.m_peerA = m_recvAddr;
				newChannel.m_lastUpdated = m_lastTickTime;

				m_guidMappedChannels.emplace(newChannel.m_guid, newChannel);

				LOG(Verbose, Relay, "Accepted new client with guid: \"{0}\"", newChannel.m_guid.toString());
			}
			else if (guidChannel->second.m_peerA != m_recvAddr && !guidChannel->second.m_peerB.isValid())
			{
				guidChannel->second.m_peerB = m_recvAddr;
				guidChannel->second.m_lastUpdated = m_lastTickTime;

				m_addressMappedChannels.emplace(guidChannel->second.m_peerA, guidChannel->second);
				m_addressMappedChannels.emplace(guidChannel->second.m_peerB, guidChannel->second);

				LOG(Info, Relay, "Channel relay established for session: \"{0}\" with peers: {1}, {2}.", nthGuid.toString(), guidChannel->second.m_peerA.toString(), guidChannel->second.m_peerB.toString());
			}
		}
	}
}

void relay::processOutcoming()
{
	while (m_sendQueue.size())
	{
		auto& pendingPacket = m_sendQueue.front();
		auto findRes = m_guidMappedChannels.find(pendingPacket.m_channelGuid);
		if (findRes != m_guidMappedChannels.end())
		{
			auto& currentChannel = findRes->second;
			const auto bytesSend = m_socket->sendTo(pendingPacket.m_buffer.data(), pendingPacket.m_buffer.size(), &pendingPacket.m_target);
			if (bytesSend <= 0)
				return;

			currentChannel.m_stats.m_packetsSent++;
			currentChannel.m_stats.m_bytesSent += bytesSend;
		}
		m_sendQueue.pop();
	}
}

channel& relay::createChannel(const guid& inGuid)
{
	channel newChannel{};
	newChannel.m_guid = inGuid;
	return m_channels.emplace_back(newChannel);
}

void relay::conditionalCleanup(bool force)
{
	uint16_t cleanupItemCount = 0;

	const auto secondsSinceLastCleanupMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_lastTickTime - m_lastCleanupTime).count();
	if (secondsSinceLastCleanupMs > m_params.m_cleanupTimeMs || force)
	{
		for (auto it = m_channels.begin(); it != m_channels.end();)
		{
			const auto timeSinceInactiveMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_lastTickTime - it->m_lastUpdated).count();
			if (timeSinceInactiveMs > m_params.m_cleanupInactiveChannelAfterMs)
			{
				const auto channelGuidStr = it->m_guid.toString();
				const auto stats = it->m_stats;
				LOG(Info, Relay, "Channel \"{0}\" inactive and removed. Relayed: {1} packets ({2} bytes); Dropped: {3} ({4})", channelGuidStr, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent);

				m_addressMappedChannels.erase(it->m_peerA);
				m_addressMappedChannels.erase(it->m_peerB);
				m_guidMappedChannels.erase(it->m_guid);

				it = m_channels.erase(it);

				++cleanupItemCount;
				if (cleanupItemCount > 32)
					break;
			}
			else
			{
				++it;
			}
		}

		m_lastCleanupTime = m_lastTickTime;
	}
}

bool relay::checkHandshakePacket(const uint8_t* buffer, const size_t bytesRead) const noexcept
{
	return bytesRead >= handshake_header_min_size;
}
