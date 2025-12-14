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
#include <map>
#include <memory>
#include <queue>

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
	uint32_t m_cleanupTimeMs{1800};
	uint32_t m_cleanupInactiveChannelAfterMs{30000};
	uint32_t m_expirePacketAfterMs{5};
	bool ipv6{};
};

const uint32_t handshake_magic_number = 0x4B28000;
const uint16_t handshake_min_size = 24;
struct alignas(4) handshake_header
{
	uint32_t m_magicNumber{handshake_magic_number};
	uint32_t m_reserved{};
	guid m_guid{};
};
static_assert(sizeof(handshake_header) == handshake_min_size);

using recv_buffer = ur::aligned_storage<alignof(std::max_align_t), 65536>;

class relay
{
public:
	relay() = default;
	relay(const relay&) = delete;
	relay(relay&&) = delete;
	~relay() = default;

	// Initialize relay with params
	bool init(relay_params params);

	// Begin running loop
	void run();

	// Immediate stop
	void stop();

	// Wait until all existing connections closed and then stop. Also prevents new connections being created.
	void stopGracefully();

private:
	void processIncoming();

	void processOutcoming();

	channel& createChannel(const guid& inGuid);

	void conditionalCleanup();

	relay_params m_params;

	internetaddr m_recvAddr{};

	recv_buffer m_recvBuffer{};

	udpsocket m_socket{};

	// when first client handshake comes, channel is created
	std::map<guid, channel> m_channels{};

	// when second client comes with same guid value, as in m_guidMappedChannels, it maps both addresses here
	std::map<internetaddr, channel&> m_addressMappedChannels{};

	// optionally used to send packets later when socket busy, if m_expirePacketAfterMs > 0
	std::queue<pending_packet> m_sendQueue{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::chrono::time_point<std::chrono::steady_clock> m_nextCleanupTime{};

	std::atomic_bool m_running{false};

	std::atomic_bool m_gracefulStopRequested{false};
};