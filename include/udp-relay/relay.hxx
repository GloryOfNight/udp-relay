// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/networking/internetaddr.hxx"
#include "udp-relay/networking/network_utils.hxx"
#include "udp-relay/networking/udpsocket.hxx"

#include "types.hxx"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <vector>

struct channel_stats
{
	uint64_t m_bytesReceived{};
	uint64_t m_bytesSent{};

	uint32_t m_packetsReceived{};
	uint32_t m_packetsSent{};
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

struct relay_params
{
	uint16_t m_primaryPort{6060};
	uint32_t m_recvBufferSize{65507};
	uint32_t m_socketRecvBufferSize{0x10000};
	uint32_t m_socketSendBufferSize{0x10000};
	int32_t m_cleanupTimeMs{1800};
	int32_t m_cleanupInactiveChannelAfterMs{30000};
	int32_t m_expirePacketAfterMs{5};
};

class relay
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

	bool checkHandshakePacket(const uint8_t* buffer, size_t bytesRead) const noexcept;

	channel& createChannel(const guid& inGuid);

	void conditionalCleanup(bool force = false);

	relay_params m_params;

	internetaddr m_recvAddr{};

	std::vector<uint8_t> m_recvBuffer{};

	// when first client handshake comes, it maps here to associate with guid
	std::map<guid, channel&> m_guidMappedChannels{};

	// when second client comes with same guid value, as in m_guidMappedChannels, it maps both addresses here
	std::map<internetaddr, channel&> m_addressMappedChannels{};

	std::queue<pending_packet> m_sendQueue{};

	std::list<channel> m_channels{};

	uniqueUdpsocket m_socket{};

	// lastTickTime used instead of now()
	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::chrono::time_point<std::chrono::steady_clock> m_nextCleanupTime{};

	std::atomic_bool m_running{false};
};