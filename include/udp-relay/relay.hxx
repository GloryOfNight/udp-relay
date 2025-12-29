// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/guid.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/socket_address.hxx"
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
	channel() = default;
	channel(guid inGuid, socket_address inPeerA, std::chrono::time_point<std::chrono::steady_clock> inLastUpdated)
		: m_guid{inGuid}
		, m_peerA{inPeerA}
		, m_lastUpdated{inLastUpdated}
	{
	}

	const guid m_guid{};
	socket_address m_peerA{};
	socket_address m_peerB{};
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

#if __cpp_lib_flat_map
#include <flat_map>
using relay_channel_map = std::flat_map<guid, channel>;
using relay_address_channel_map = std::flat_map<socket_address, guid>;
#else
using relay_channel_map = std::map<guid, channel>;
using relay_address_channel_map = std::map<socket_address, guid>;
#endif

constexpr uint32_t handshake_magic_number_host = 0x4B28000;
constexpr uint32_t handshake_magic_number_hton = ur::net::hton32(0x4B28000);
constexpr uint16_t handshake_min_size = 24;
struct alignas(4) handshake_header
{
	uint32_t m_magicNumber{handshake_magic_number_host};
	uint32_t m_reserved{};
	guid m_guid{};
};
static_assert(sizeof(handshake_header) == handshake_min_size);

class relay
{
public:
	using recv_buffer = std::array<std::uint64_t, 65536 / alignof(std::uint64_t)>;

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

	socket_address m_recvAddr{};

	recv_buffer m_recvBuffer{};

	udpsocket m_socket{};

	relay_channel_map m_channels{};

	relay_address_channel_map m_addressChannels{};

	std::chrono::steady_clock::time_point m_lastTickTime{};

	std::chrono::steady_clock::time_point m_nextCleanupTime{};

	std::atomic_bool m_running{false};

	std::atomic_bool m_gracefulStopRequested{false};
};

struct relay_helpers
{
	static std::pair<bool, handshake_header> tryDeserializeHeader(relay::recv_buffer& recvBuffer, size_t recvBytes);
};