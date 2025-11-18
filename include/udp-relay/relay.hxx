// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/aligned_storage.hxx"
#include "udp-relay/guid.hxx"
#include "udp-relay/net/internetaddr.hxx"
#include "udp-relay/net/udpsocket.hxx"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <queue>
#include <unordered_map>

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

struct relay_params
{
	uint16_t m_primaryPort{6060};
	uint32_t m_socketRecvBufferSize{0};
	uint32_t m_socketSendBufferSize{0};
	int32_t m_cleanupTimeMs{1800};
	int32_t m_cleanupInactiveChannelAfterMs{30000};
	int32_t m_expirePacketAfterMs{5};
	bool ipv6{};
};

const uint32_t handshake_header_magic_number = 0x4B28000;
const uint16_t handshake_header_min_size = 24;
struct alignas(8) handshake_header
{
	uint32_t m_magicNumber{handshake_header_magic_number};
	uint32_t m_reserved{};
	guid m_guid{};
};
static_assert(sizeof(handshake_header) == handshake_header_min_size);

using recvBufferStorage = ur::aligned_storage<alignof(std::max_align_t), 65536>;

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

	channel& createChannel(const guid& inGuid);

	void conditionalCleanup(bool force = false);

	relay_params m_params;

	internetaddr m_recvAddr{};

	recvBufferStorage m_recvBuffer{};

	// when first client handshake comes, channel is created
	std::unordered_map<guid, channel> m_channels{};

	// when second client comes with same guid value, as in m_guidMappedChannels, it maps both addresses here
	std::unordered_map<internetaddr, channel&> m_addressMappedChannels{};

	std::queue<pending_packet> m_sendQueue{};

	uniqueUdpsocket m_socket{};

	// lastTickTime used instead of now()
	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::chrono::time_point<std::chrono::steady_clock> m_nextCleanupTime{};

	std::atomic_bool m_running{false};
};