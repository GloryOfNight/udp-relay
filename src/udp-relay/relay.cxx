// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/version.hxx"

using namespace std::chrono_literals;

std::pair<bool, handshake_header> relay_helpers::tryDeserializeHeader(relay::recv_buffer& recvBuffer, size_t recvBytes)
{
	if (recvBytes < sizeof(handshake_header))
		return std::pair<bool, handshake_header>();

	handshake_header recvHeader{};
	std::memcpy(&recvHeader, recvBuffer.data(), sizeof(recvHeader));

	if (recvHeader.m_magicNumber != handshake_magic_number_hton)
		return std::pair<bool, handshake_header>();

	constexpr guid zeroGuid{};
	if (recvHeader.m_guid == zeroGuid)
		return std::pair<bool, handshake_header>();

	recvHeader.m_magicNumber = ur::net::ntoh32(recvHeader.m_magicNumber);
	recvHeader.m_guid.m_a = ur::net::ntoh32(recvHeader.m_guid.m_a);
	recvHeader.m_guid.m_b = ur::net::ntoh32(recvHeader.m_guid.m_b);
	recvHeader.m_guid.m_c = ur::net::ntoh32(recvHeader.m_guid.m_c);
	recvHeader.m_guid.m_d = ur::net::ntoh32(recvHeader.m_guid.m_d);

	return std::pair<bool, handshake_header>{true, recvHeader};
}

bool relay::init(relay_params params)
{
	if (m_running)
	{
		LOG(Warning, Relay, "Cannon initialize while running");
		return false;
	}

	LOG(Verbose, Relay, "Begin initialization");

	udpsocket newSocket = udpsocket::make(params.ipv6);
	if (!newSocket.isValid())
	{
		LOG(Error, Relay, "Failed to create socket!");
		return false;
	}

	if (params.ipv6 && !newSocket.setOnlyIpv6(false))
	{
		LOG(Error, Relay, "Failed set socket ipv6 to dual-stack mode");
		return false;
	}

	const auto bindAddr = params.ipv6 ? socket_address::make_ipv6(ur::net::anyIpv6(), params.m_primaryPort) : socket_address::make_ipv4(ur::net::anyIpv4(), params.m_primaryPort);
	if (!newSocket.bind(bindAddr))
	{
		LOG(Error, Relay, "Failed bind socket to {}", bindAddr);
		return false;
	}

	if (!newSocket.setNonBlocking(true))
	{
		LOG(Error, Relay, "Failed set socket to non-blocking mode");
		return false;
	}

	if (params.m_socketSendBufferSize)
	{
		if (!newSocket.setSendBufferSize(params.m_socketSendBufferSize))
			LOG(Warning, Relay, "Failed set send buffer size to {}", params.m_socketSendBufferSize);
		LOG(Info, Relay, "Socket requested send buffer size {}", params.m_socketSendBufferSize);
	}

	if (params.m_socketRecvBufferSize)
	{
		if (!newSocket.setRecvBufferSize(params.m_socketRecvBufferSize))
			LOG(Warning, Relay, "Failed set recv buffer size to {}", params.m_socketRecvBufferSize);
		LOG(Info, Relay, "Socket requested recv buffer size {}", params.m_socketRecvBufferSize);
	}

	LOG(Info, Relay, "Relay initialized {:A}:{}. SndBuf={}, RcvBuf={}. Version: {}.{}.{}",
		bindAddr, newSocket.getPort(), newSocket.getSendBufferSize(), newSocket.getRecvBufferSize(),
		ur::getVersionMajor(), ur::getVersionMinor(), ur::getVersionPatch());

	m_params = params;
	m_socket = std::move(newSocket);

	return true;
}

void relay::run()
{
	if (!m_socket.isValid())
	{
		LOG(Error, Relay, "Cannot run while not initialized");
		return;
	}

	m_running = true;

	constexpr size_t preAllocateNum = 2048;
	constexpr size_t preAllocateNumX2 = preAllocateNum * 2;
	{ // pre-allocate channel space
		decltype(m_channels)::key_container_type keys;
		keys.reserve(preAllocateNum);
		decltype(m_channels)::mapped_container_type values;
		values.reserve(preAllocateNum);
		m_channels.replace(std::move(keys), std::move(values));
	}
	{ // pre-allocate addressChannel space
		decltype(m_addressChannels)::key_container_type keys;
		keys.reserve(preAllocateNumX2);
		decltype(m_addressChannels)::mapped_container_type values;
		values.reserve(preAllocateNumX2);
		m_addressChannels.replace(std::move(keys), std::move(values));
	}

	while (m_running)
	{
		m_lastTickTime = std::chrono::steady_clock::now();

		m_socket.waitForWrite(1000us);
		if (m_socket.waitForRead(15000us))
			processIncoming();

		conditionalCleanup();

		if (m_gracefulStopRequested && m_channels.size() == 0)
		{
			stop();
		}
	}

	LOG(Info, Relay, "Exited run loop");
}

void relay::stop()
{
	LOG(Info, Relay, "Stop");
	m_running = false;
}

void relay::stopGracefully()
{
	LOG(Info, Relay, "Graceful stop requested");
	m_gracefulStopRequested = true;
}

void relay::processIncoming()
{
	const int32_t maxRecvCycles = 32;
	for (int32_t currentCycle = 0; currentCycle < maxRecvCycles; ++currentCycle)
	{
		int32_t bytesRead{};
		bytesRead = m_socket.recvFrom(m_recvBuffer.data(), m_recvBuffer.size(), m_recvAddr);
		if (bytesRead < 0)
			return;

		// check if packet is relay handshake header, and extract guid if possible
		// always check for handshake first, to allow creating new connections from same socket without waiting prev. session to close
		if (const auto [isHeader, header] = relay_helpers::tryDeserializeHeader(m_recvBuffer, bytesRead); !m_gracefulStopRequested && isHeader)
		{
			auto findChannel = m_channels.find(header.m_guid);
			if (findChannel == m_channels.end())
			{
				// clang-format off
				const channel newChannel = channel
				{
					.m_guid = header.m_guid,
					.m_peerA = m_recvAddr,
					.m_lastUpdated = m_lastTickTime
				};
				// clang-format on
				m_channels.emplace(header.m_guid, newChannel);

				LOG(Info, Relay, "Channel allocated: \"{}\". Peer: {}", header.m_guid, m_recvAddr);
			}
			else if (findChannel->second.m_peerA != m_recvAddr && findChannel->second.m_peerB.isNull())
			{
				findChannel->second.m_peerB = m_recvAddr;
				findChannel->second.m_lastUpdated = m_lastTickTime;

				m_addressChannels.emplace(findChannel->second.m_peerA, findChannel->second.m_guid);
				m_addressChannels.emplace(findChannel->second.m_peerB, findChannel->second.m_guid);

				LOG(Info, Relay, "Channel established: \"{}\". PeerA: {}, PeerB: {}", header.m_guid, findChannel->second.m_peerA, findChannel->second.m_peerB);
			}
		}

		// if recvAddr has a channel mapped to it, as well as two valid peers, relay packet to the other peer
		const auto findAddressChannel = m_addressChannels.find(m_recvAddr);
		if (findAddressChannel != m_addressChannels.end())
		{
			const auto& findChannel = m_channels.find(findAddressChannel->second);
			if (findChannel == m_channels.end()) [[unlikely]]
				continue;

			auto& currentChannel = findChannel->second;

			currentChannel.m_lastUpdated = m_lastTickTime;

			currentChannel.m_stats.m_packetsReceived++;
			currentChannel.m_stats.m_bytesReceived += bytesRead;

			const auto& sendAddr = currentChannel.m_peerA != m_recvAddr ? currentChannel.m_peerA : currentChannel.m_peerB;

			// try relay packet immediately
			const auto bytesSend = m_socket.sendTo(m_recvBuffer.data(), bytesRead, sendAddr);
			if (bytesSend >= 0)
			{
				currentChannel.m_stats.m_packetsSent++;
				currentChannel.m_stats.m_bytesSent += bytesSend;
			}
		}
	}
}

void relay::conditionalCleanup()
{
	if (m_lastTickTime < m_nextCleanupTime)
		return;

	{ // close inactive channels
		const auto eraseChannelLam = [&](const auto& pair) -> bool
		{
			const auto timeSinceInactive = m_lastTickTime - pair.second.m_lastUpdated;
			if (timeSinceInactive > m_params.m_cleanupInactiveChannelAfterTime)
			{
				const auto& stats = pair.second.m_stats;
				LOG(Info, Relay, "Channel closed: \"{0}\". Received: {1} packets ({2} bytes); Dropped: {3} ({4});",
					pair.second.m_guid, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent);
				return true;
			}
			return false;
		};
		std::erase_if(m_channels, eraseChannelLam);
	}

	{ // erase address mappings that uses erased channels
		const auto eraseAddressChannelLam = [&](const auto& pair) -> bool
		{
			return m_channels.find(pair.second) == m_channels.end();
		};
		std::erase_if(m_addressChannels, eraseAddressChannelLam);
	}

	m_nextCleanupTime = m_lastTickTime + m_params.m_cleanupTime;

	ur::log_flush();
}
