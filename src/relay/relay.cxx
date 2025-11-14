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

	if (params.m_socketSendBufferSize)
	{
		if (!newSocket->setSendBufferSize(params.m_socketSendBufferSize))
			LOG(Warning, Relay, "Failed set send buffer size to {}", params.m_socketSendBufferSize);
		LOG(Info, Relay, "Socket set send buffer size {} requested", params.m_socketSendBufferSize);
	}

	if (params.m_socketRecvBufferSize)
	{
		if (!newSocket->setRecvBufferSize(params.m_socketRecvBufferSize))
			LOG(Warning, Relay, "Failed set recv buffer size to {}", params.m_socketRecvBufferSize);
		LOG(Info, Relay, "Socket set recv buffer size {} requested", params.m_socketRecvBufferSize);
	}

	LOG(Info, Relay, "Relay initialized on port {}. SndBuf={}, RcvBuf={}", newSocket->getPort(), newSocket->getSendBufferSize(), newSocket->getRecvBufferSize());

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

	m_running = true;

	while (m_running)
	{
		m_lastTickTime = std::chrono::steady_clock::now();

		if (m_socket->waitForWriteUs(1000))
			processOutcoming();

		if (m_socket->waitForReadUs(15000))
			processIncoming();

		conditionalCleanup();
	}
}

void relay::stop()
{
	m_running = false;
}

void relay::processIncoming()
{
	const int32_t maxRecvCycles = 32;
	for (int32_t currentCycle = 0; currentCycle < maxRecvCycles; ++currentCycle)
	{
		int32_t bytesRead{};
		bytesRead = m_socket->recvFrom(m_recvBuffer.data(), m_recvBuffer.size(), &m_recvAddr);
		if (bytesRead < 0)
			return;

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
			if (bytesSend >= 0)
			{
				currentChannel.m_stats.m_packetsSent++;
				currentChannel.m_stats.m_bytesSent += bytesSend;
			}
			else if (m_params.m_expirePacketAfterMs)
			{
				pending_packet pendingPacket = pending_packet();
				pendingPacket.m_channelGuid = currentChannel.m_guid;
				pendingPacket.m_target = sendAddr;
				pendingPacket.m_expireAt = m_lastTickTime + std::chrono::milliseconds(m_params.m_expirePacketAfterMs);
				pendingPacket.m_buffer.resize(bytesRead);
				std::memcpy(pendingPacket.m_buffer.data(), m_recvBuffer.data(), bytesRead);

				m_sendQueue.push(std::move(pendingPacket));
			}
		}
		// check if packet is relay handshake header, and extract guid if possible
		else if (const auto& [bOk, header] = relay_helpers::deserializePacket(m_recvBuffer.data(), bytesRead); bOk)
		{
			if (header.m_magicNumber != handshake_header_magic_number && header.m_guid.isValid())
				continue;

			auto findChannel = m_channels.find(header.m_guid);
			if (findChannel == m_channels.end())
			{
				channel newChannel = channel(header.m_guid);
				newChannel.m_peerA = m_recvAddr;
				newChannel.m_lastUpdated = m_lastTickTime;

				m_channels.emplace(header.m_guid, std::move(newChannel));

				LOG(Verbose, Relay, "Accepted new client with guid: \"{0}\"", newChannel.m_guid.toString());
			}
			else if (findChannel->second.m_peerA != m_recvAddr && !findChannel->second.m_peerB.isValid())
			{
				findChannel->second.m_peerB = m_recvAddr;
				findChannel->second.m_lastUpdated = m_lastTickTime;

				m_addressMappedChannels.emplace(findChannel->second.m_peerA, findChannel->second);
				m_addressMappedChannels.emplace(findChannel->second.m_peerB, findChannel->second);

				LOG(Info, Relay, "Channel relay established for session: \"{0}\" with peers: {1}, {2}.", header.m_guid.toString(), findChannel->second.m_peerA.toString(), findChannel->second.m_peerB.toString());
			}
		}
	}
}

void relay::processOutcoming()
{
	while (m_sendQueue.size())
	{
		auto& pendingPacket = m_sendQueue.front();
		const bool packetWithinExpireLimit = pendingPacket.m_expireAt > m_lastTickTime;

		auto findChannel = m_channels.find(pendingPacket.m_channelGuid);
		if (packetWithinExpireLimit && findChannel != m_channels.end())
		{
			auto& currentChannel = findChannel->second;
			const auto bytesSend = m_socket->sendTo(pendingPacket.m_buffer.data(), pendingPacket.m_buffer.size(), &pendingPacket.m_target);
			if (bytesSend <= 0)
				return;

			currentChannel.m_stats.m_packetsSent++;
			currentChannel.m_stats.m_bytesSent += bytesSend;
		}

		m_sendQueue.pop();
	}
}

void relay::conditionalCleanup(bool force)
{
	if (m_lastTickTime >= m_nextCleanupTime || force)
	{
		for (auto it = m_channels.begin(); it != m_channels.end();)
		{
			const auto timeSinceInactiveMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_lastTickTime - it->second.m_lastUpdated).count();
			if (timeSinceInactiveMs > m_params.m_cleanupInactiveChannelAfterMs)
			{
				const auto channelGuidStr = it->second.m_guid.toString();
				const auto stats = it->second.m_stats;
				LOG(Info, Relay, "Channel \"{0}\" inactive and removed. Relayed: {1} packets ({2} bytes); Dropped: {3} ({4})", channelGuidStr, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent);

				m_addressMappedChannels.erase(it->second.m_peerA);
				m_addressMappedChannels.erase(it->second.m_peerB);

				it = m_channels.erase(it);
			}
			else
			{
				++it;
			}
		}

		LOG_FLUSH();

		m_nextCleanupTime = m_lastTickTime + std::chrono::milliseconds(m_params.m_cleanupTimeMs);
	}
}

std::pair<bool, handshake_header> relay_helpers::deserializePacket(const uint8_t* buffer, const size_t len)
{
	if (len < sizeof(handshake_header))
		return std::pair<bool, handshake_header>();

	handshake_header netHeader;
	std::memcpy(&netHeader, buffer, sizeof(handshake_header));

	handshake_header hostHeader;
	hostHeader.m_magicNumber = ur::net::ntoh32(netHeader.m_magicNumber);
	hostHeader.m_guid.m_a = ur::net::ntoh32(netHeader.m_guid.m_a);
	hostHeader.m_guid.m_b = ur::net::ntoh32(netHeader.m_guid.m_b);
	hostHeader.m_guid.m_c = ur::net::ntoh32(netHeader.m_guid.m_c);
	hostHeader.m_guid.m_d = ur::net::ntoh32(netHeader.m_guid.m_d);
	return std::pair<bool, handshake_header>(true, hostHeader);
}