// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/udpsocket.hxx"

#include <array>

bool relay::run(const relay_params& params)
{
	m_params = params;

	if (!init())
		return false;

	LOG(Log, "Relay initialized. Requested {0} port, actual {1}", m_params.m_primaryPort, m_socket->getPort());
	LOG(Log, "Relay tick time warning set to {0}us", m_params.m_warnTickExceedTimeUs);

	packet_buffer buffer{};
	internetaddr recvAddr{};

	m_lastTickTime = std::chrono::steady_clock::now();
	m_lastCleanupTime = m_lastTickTime;
	m_running = true;

	while (m_running)
	{
		if (m_pendingPackets.size() == 0)
			m_socket->waitForReadUs(10000);
		else
			m_socket->waitForWriteUs(100);

		m_lastTickTime = std::chrono::steady_clock::now();

		const int32_t bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), &recvAddr);
		if (bytesRead > 0)
		{
			const auto findRes = m_addressMappedChannels.find(recvAddr);
			// if recvAddr has a channel mapped to it, as well as two valid peers, relay packet to the other peer
			if (findRes != m_addressMappedChannels.end() && findRes->second.m_peerA.isValid() && findRes->second.m_peerB.isValid())
			{
				auto& currentChannel = findRes->second;

				currentChannel.m_lastUpdated = m_lastTickTime;

				currentChannel.m_stats.m_packetsReceived++;
				currentChannel.m_stats.m_bytesReceived += bytesRead;

				const auto& sendAddr = findRes->second.m_peerA != recvAddr ? findRes->second.m_peerA : findRes->second.m_peerB;

				const auto bytesSend = m_socket->sendTo(buffer.data(), bytesRead, &sendAddr);
				if (bytesSend > 0) // if send fails, reattempt re-send on future ticks
				{
					currentChannel.m_stats.m_packetsSent++;
					currentChannel.m_stats.m_bytesSent += bytesSend;
				}
				else
				{
					m_pendingPackets.push(pending_packet{currentChannel.m_guid, sendAddr, buffer, bytesRead});
				}
			}
			else if (checkHandshakePacket(buffer, bytesRead))
			{
				const auto& recvGuid = reinterpret_cast<const handshake_header*>(buffer.data())->m_guid;
				const guid nthGuid = guid(BYTESWAP32(recvGuid.m_a), BYTESWAP32(recvGuid.m_b), BYTESWAP32(recvGuid.m_c), BYTESWAP32(recvGuid.m_d));

				auto guidChannel = m_guidMappedChannels.find(nthGuid);
				if (guidChannel == m_guidMappedChannels.end())
				{
					channel& newChannel = createChannel(nthGuid);
					newChannel.m_peerA = recvAddr;
					newChannel.m_lastUpdated = m_lastTickTime;

					m_guidMappedChannels.emplace(newChannel.m_guid, newChannel);

					LOG(Verbose, "Created channel for guid: \"{0}\"", newChannel.m_guid.toString());
				}
				else if (guidChannel->second.m_peerA != recvAddr && !guidChannel->second.m_peerB.isValid())
				{
					guidChannel->second.m_peerB = recvAddr;
					guidChannel->second.m_lastUpdated = m_lastTickTime;

					m_addressMappedChannels.emplace(guidChannel->second.m_peerA, guidChannel->second);
					m_addressMappedChannels.emplace(guidChannel->second.m_peerB, guidChannel->second);

					LOG(Log, "Channel relay established for session: \"{0}\" with peers: {1}, {2}.", nthGuid.toString(), guidChannel->second.m_peerA.toString(), guidChannel->second.m_peerB.toString());
				}
			}
		}

		if (m_pendingPackets.size())
			sendPendingPackets();

		conditionalCleanup(false);
		checkWarnLogTickTime();
	}

	return true;
}

void relay::stop()
{
	m_running = false;
}

bool relay::init()
{
	LOG(Verbose, "Begin initialization");

	m_socket = udpsocketFactory::createUdpSocket();
	if (m_socket == nullptr || !m_socket->isValid())
	{
		LOG(Error, "Failed to create socket!");
		return false;
	}

	LOG(Verbose, "Created primary udp socket");

	if (!m_socket->bind(m_params.m_primaryPort))
	{
		LOG(Error, "Failed bind to {0} port", m_params.m_primaryPort);
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

void relay::sendPendingPackets()
{
	while (m_pendingPackets.size())
	{
		auto& pendingPacket = m_pendingPackets.front();
		auto findRes = m_guidMappedChannels.find(pendingPacket.m_guid);
		if (findRes != m_guidMappedChannels.end())
		{
			auto& currentChannel = findRes->second;
			const auto bytesSend = m_socket->sendTo(pendingPacket.m_buffer.data(), pendingPacket.m_bytesRead, &pendingPacket.m_target);
			if (bytesSend <= 0)
				return;

			currentChannel.m_stats.m_packetsSent++;
			currentChannel.m_stats.m_bytesSent += bytesSend;
		}
		m_pendingPackets.pop();
	}
}

void relay::conditionalCleanup(bool force)
{
	uint16_t cleanupItemCount = 0;

	const auto secondsSinceLastCleanupMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_lastTickTime - m_lastCleanupTime).count();
	if (secondsSinceLastCleanupMs > 1800 || force)
	{
		for (auto it = m_channels.begin(); it != m_channels.end();)
		{
			const auto timeSinceInactiveMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_lastTickTime - it->m_lastUpdated).count();
			if (timeSinceInactiveMs > 30000)
			{
				const auto channelGuidStr = it->m_guid.toString();
				const auto stats = it->m_stats;
				LOG(Log, "Channel \"{0}\" inactive and removed. Relayed: {1} packets ({2} bytes); Dropped: {3} ({4})", channelGuidStr, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent);

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

void relay::checkWarnLogTickTime()
{
	if (log_level::Warning > g_logLevel)
		return;

	const auto now = std::chrono::steady_clock::now();
	const int64_t timeSinceLastTick = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastTickTime).count();
	if (timeSinceLastTick > m_params.m_warnTickExceedTimeUs)
	{
		LOG(Warning, "Tick {0}us time exceeded limit {1}us", timeSinceLastTick, m_params.m_warnTickExceedTimeUs);
	}
}
