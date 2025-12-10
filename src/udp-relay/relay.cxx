// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"
module;

#include "udp-relay/log.hxx"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <queue>
#include <thread>

export module ur.relay;

import ur.version;
import ur.log;
import ur.net.utils;
import ur.aligned_storage;
import ur.guid;
import ur.net.internetaddr;
import ur.net.udpsocket;

struct channel_stats
{
	uint64_t m_bytesReceived{};
	uint64_t m_bytesSent{};

	uint32_t m_packetsReceived{};
	uint32_t m_packetsSent{};

	uint32_t m_sendPacketDelays{};
};

struct channel
{
	channel() = default;
	channel(const guid& inGuid)
		: m_guid{inGuid}
	{
	}

	const guid m_guid{};
	internetaddr m_peerA{};
	internetaddr m_peerB{};
	std::chrono::time_point<std::chrono::steady_clock> m_lastUpdated{};
	channel_stats m_stats{};
};

struct pending_packet
{
	guid m_channelGuid{};
	internetaddr m_target{};
	std::chrono::time_point<std::chrono::steady_clock> m_expireAt{};
	std::vector<uint8_t> m_buffer{};
};

export struct relay_params
{
	uint16_t m_primaryPort{6060};
	uint32_t m_socketRecvBufferSize{0};
	uint32_t m_socketSendBufferSize{0};
	int32_t m_cleanupTimeMs{1800};
	int32_t m_cleanupInactiveChannelAfterMs{30000};
	int32_t m_expirePacketAfterMs{5};
	bool ipv6{};
};

export const uint32_t handshake_magic_number = 0x4B28000;
export const uint16_t handshake_min_size = 24;
export struct alignas(4) handshake_header
{
	uint32_t m_magicNumber{handshake_magic_number};
	uint32_t m_reserved{};
	guid m_guid{};
};
static_assert(sizeof(handshake_header) == handshake_min_size);

using recvBufferStorage = ur::aligned_storage<alignof(std::max_align_t), 65536>;

export class relay
{
public:
	relay() = default;
	relay(const relay&) = delete;
	relay(relay&&) = delete;
	~relay() = default;

	bool init(const relay_params& params);

	void run();

	void stop();

private:
	void processIncoming();

	void processOutcoming();

	channel& createChannel(const guid& inGuid);

	void conditionalCleanup(bool force = false);

	relay_params m_params;

	internetaddr m_recvAddr{};

	recvBufferStorage m_recvBuffer{};

	// when first client handshake comes, channel is created
	std::map<guid, channel> m_channels{};

	// when second client comes with same guid value, as in m_guidMappedChannels, it maps both addresses here
	std::map<internetaddr, channel&> m_addressMappedChannels{};

	std::queue<pending_packet> m_sendQueue{};

	uniqueUdpsocket m_socket{};

	// lastTickTime used instead of now()
	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::chrono::time_point<std::chrono::steady_clock> m_nextCleanupTime{};

	std::atomic_bool m_running{false};
};

struct relay_helpers
{
	static handshake_header* tryDeserializeHeader(recvBufferStorage& recvBuffer, size_t recvBytes);
};

module :private;

handshake_header* relay_helpers::tryDeserializeHeader(recvBufferStorage& recvBuffer, size_t recvBytes)
{
	if (recvBytes < sizeof(handshake_header))
		return nullptr;

	static const uint32_t handshakeMagicNumberHton = ur::net::hton32(handshake_magic_number);

	handshake_header* recvHeader = reinterpret_cast<handshake_header*>(recvBuffer.data());
	if (recvHeader->m_magicNumber != handshakeMagicNumberHton || recvHeader->m_guid.isNull())
		return nullptr;

	recvHeader->m_magicNumber = ur::net::ntoh32(recvHeader->m_magicNumber);
	recvHeader->m_guid.m_c = ur::net::ntoh32(recvHeader->m_guid.m_c);
	recvHeader->m_guid.m_d = ur::net::ntoh32(recvHeader->m_guid.m_d);
	recvHeader->m_guid.m_b = ur::net::ntoh32(recvHeader->m_guid.m_b);
	recvHeader->m_guid.m_a = ur::net::ntoh32(recvHeader->m_guid.m_a);
	return recvHeader;
}

bool relay::init(const relay_params& params)
{
	LOG(Verbose, Relay, "Begin initialization");

	uniqueUdpsocket newSocket = std::make_unique<udpsocket>(params.ipv6);
	if (newSocket == nullptr || !newSocket->isValid())
	{
		LOG(Error, Relay, "Failed to create socket!");
		return false;
	}

	if (params.ipv6 && !newSocket->setOnlyIpv6(false))
	{
		LOG(Error, Relay, "Failed set socket ipv6 to dual-stack mode");
		return false;
	}

	const auto addr = params.ipv6 ? internetaddr::make_ipv6(ur::net::anyIpv6(), params.m_primaryPort) : internetaddr::make_ipv4(ur::net::anyIpv4(), params.m_primaryPort);
	if (!newSocket->bind(addr))
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
		LOG(Info, Relay, "Socket requested send buffer size {}", params.m_socketSendBufferSize);
	}

	if (params.m_socketRecvBufferSize)
	{
		if (!newSocket->setRecvBufferSize(params.m_socketRecvBufferSize))
			LOG(Warning, Relay, "Failed set recv buffer size to {}", params.m_socketRecvBufferSize);
		LOG(Info, Relay, "Socket requested recv buffer size {}", params.m_socketRecvBufferSize);
	}

	LOG(Info, Relay, "Relay initialized {}:{}. SndBuf={}, RcvBuf={}. Version: {}.{}.{}",
		addr.toString(false), newSocket->getPort(), newSocket->getSendBufferSize(), newSocket->getRecvBufferSize(),
		ur::getVersionMajor(), ur::getVersionMinor(), ur::getVersionPatch());

	m_params = params;
	m_socket = std::move(newSocket);

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

		if (m_sendQueue.size() && m_socket->waitForWriteUs(1000))
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

		// check if packet is relay handshake header, and extract guid if possible
		if (auto recvHeader = relay_helpers::tryDeserializeHeader(m_recvBuffer, bytesRead); recvHeader)
		{
			handshake_header& recvHeaderRef = *recvHeader;

			auto findChannel = m_channels.find(recvHeaderRef.m_guid);
			if (findChannel == m_channels.end())
			{
				LOG(Verbose, Relay, "Accepted new client with guid: \"{0}\"", recvHeaderRef.m_guid.toString());

				channel newChannel = channel(recvHeaderRef.m_guid);
				newChannel.m_peerA = m_recvAddr;
				newChannel.m_lastUpdated = m_lastTickTime;

				m_channels.emplace(recvHeaderRef.m_guid, std::move(newChannel));
			}
			else if (findChannel->second.m_peerA != m_recvAddr && findChannel->second.m_peerB.isNull())
			{
				findChannel->second.m_peerB = m_recvAddr;
				findChannel->second.m_lastUpdated = m_lastTickTime;

				m_addressMappedChannels.emplace(findChannel->second.m_peerA, findChannel->second);
				m_addressMappedChannels.emplace(findChannel->second.m_peerB, findChannel->second);

				LOG(Info, Relay, "Channel relay established for session: \"{0}\" with peers: {1}, {2}.", recvHeaderRef.m_guid.toString(), findChannel->second.m_peerA.toString(), findChannel->second.m_peerB.toString());
			}
		}

		// if recvAddr has a channel mapped to it, as well as two valid peers, relay packet to the other peer
		const auto findRes = m_addressMappedChannels.find(m_recvAddr);
		if (findRes != m_addressMappedChannels.end() && !findRes->second.m_peerA.isNull() && !findRes->second.m_peerB.isNull())
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
				auto& pendingPacket = m_sendQueue.emplace();
				pendingPacket.m_channelGuid = currentChannel.m_guid;
				pendingPacket.m_target = sendAddr;
				pendingPacket.m_expireAt = m_lastTickTime + std::chrono::milliseconds(m_params.m_expirePacketAfterMs);
				pendingPacket.m_buffer.resize(bytesRead);
				std::memcpy(pendingPacket.m_buffer.data(), m_recvBuffer.data(), bytesRead);

				currentChannel.m_stats.m_sendPacketDelays++;
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
			if (bytesSend < 0)
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
				LOG(Info, Relay, "Channel \"{0}\" inactive and removed. Relayed: {1} packets ({2} bytes); Dropped: {3} ({4}); Delayed {5};", channelGuidStr, stats.m_packetsReceived, stats.m_bytesReceived, stats.m_packetsReceived - stats.m_packetsSent, stats.m_bytesReceived - stats.m_bytesSent, stats.m_sendPacketDelays);

				m_addressMappedChannels.erase(it->second.m_peerA);
				m_addressMappedChannels.erase(it->second.m_peerB);

				it = m_channels.erase(it);
			}
			else
			{
				++it;
			}
		}

		m_nextCleanupTime = m_lastTickTime + std::chrono::milliseconds(m_params.m_cleanupTimeMs);
	}
}
