// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

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
};

struct channel
{
	const guid m_guid{};
	internetaddr m_peerA{};
	internetaddr m_peerB{};
	std::chrono::time_point<std::chrono::steady_clock> m_lastUpdated{};
	channel_stats m_stats{};
};

struct relay_params
{
	uint16_t m_primaryPort{6060};
	uint32_t m_socketRecvBufferSize{0};
	uint32_t m_socketSendBufferSize{0};
	std::chrono::milliseconds m_cleanupTime{1800};
	std::chrono::milliseconds m_cleanupInactiveChannelAfterTime{30000};
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

class relay
{
public:
	using recv_buffer = std::array<std::max_align_t, 65536 / alignof(std::max_align_t)>;

	relay() = default;
	relay(const relay&) = delete;
	relay(relay&&) = delete;
	~relay() = default;

	// Initialize relay with params
	bool init(relay_params params);

	// Begin spin loop
	void run();

	// Immediate stop
	void stop();

	// Wait until all existing connections closed and then stop. Also prevents new connections being created.
	void stopGracefully();

private:
	void processIncoming();

	void conditionalCleanup();

	relay_params m_params;

	internetaddr m_recvAddr{};

	recv_buffer m_recvBuffer{};

	udpsocket m_socket{};

	std::map<guid, channel> m_channels{};

	std::map<internetaddr, guid> m_addressChannels{};

	std::chrono::steady_clock::time_point m_lastTickTime{};

	std::chrono::steady_clock::time_point m_nextCleanupTime{};

	std::atomic_bool m_running{false};

	std::atomic_bool m_gracefulStopRequested{false};
};