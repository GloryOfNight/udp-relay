// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/version.hxx"

#include <thread>

using namespace std::chrono_literals;

struct relay_helpers
{
	static std::pair<bool, handshake_header> tryDeserializeHeader(relay::recv_buffer& recvBuffer, size_t recvBytes);
};

std::pair<bool, handshake_header> relay_helpers::tryDeserializeHeader(relay::recv_buffer& recvBuffer, size_t recvBytes)
{
	if (recvBytes < sizeof(handshake_header))
		return std::pair<bool, handshake_header>();

	constexpr uint32_t handshakeMagicNumberHton = ur::net::hton32(handshake_magic_number);
	if (std::memcmp(&handshakeMagicNumberHton, (std::byte*)recvBuffer.data() + offsetof(handshake_header, m_magicNumber), sizeof(uint32_t)) != 0)
		return std::pair<bool, handshake_header>();

	constexpr guid zeroGuid{};
	if (std::memcmp(&zeroGuid, (std::byte*)recvBuffer.data() + offsetof(handshake_header, m_guid), sizeof(guid)) == 0)
		return std::pair<bool, handshake_header>();

	handshake_header recvHeader;
	std::memcpy(&recvHeader, recvBuffer.data(), sizeof(handshake_header));

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

	const auto addr = params.ipv6 ? internetaddr::make_ipv6(ur::net::anyIpv6(), params.m_primaryPort) : internetaddr::make_ipv4(ur::net::anyIpv4(), params.m_primaryPort);
	if (!newSocket.bind(addr))
	{
		LOG(Error, Relay, "Failed bind to {0} port", params.m_primaryPort);
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
		addr, newSocket.getPort(), newSocket.getSendBufferSize(), newSocket.getRecvBufferSize(),
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
		if (const auto [isHeader, header] = relay_helpers::tryDeserializeHeader(m_recvBuffer, bytesRead); !m_gracefulStopRequested && isHeader)
		{
			auto findChannel = m_channels.find(header.m_guid);
			if (findChannel == m_channels.end())
			{
				LOG(Verbose, Relay, "Accepted new client with guid: \"{}\"", header.m_guid);

				channel newChannel = channel(header.m_guid);
				newChannel.m_peerA = m_recvAddr;
				newChannel.m_lastUpdated = m_lastTickTime;

				m_channels.emplace(header.m_guid, std::move(newChannel));
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

	for (auto it = m_channels.begin(); it != m_channels.end();)
	{
		const auto timeSinceInactive = m_lastTickTime - it->second.m_lastUpdated;
		if (timeSinceInactive > m_params.m_cleanupInactiveChannelAfterTime)
		{
			const auto stats = it->second.m_stats;
			LOG(Info, Relay, "Channel closed: \"{0}\". Received: {1} packets ({2} bytes); Dropped: {3} ({4});",
				it->second.m_guid, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent);

			it = m_channels.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = m_addressChannels.begin(); it != m_addressChannels.end();)
	{
		if (m_channels.find(it->second) == m_channels.end())
			it = m_addressChannels.erase(it);
		else
			++it;
	}

	m_nextCleanupTime = m_lastTickTime + m_params.m_cleanupTime;

	ur::log_flush();
}
