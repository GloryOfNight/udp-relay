// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/guid.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/socket_address.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/ring_buffer.hxx"

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
};

struct channel
{
	channel() = default;
	channel(guid inGuid, socket_address inPeerA, std::chrono::steady_clock::time_point inLastUpdated)
		: m_guid{inGuid}
		, m_peerA{inPeerA}
		, m_lastUpdated{inLastUpdated}
	{
	}

	const guid m_guid{};
	socket_address m_peerA{};
	socket_address m_peerB{};
	std::chrono::steady_clock::time_point m_lastUpdated{};
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

using hmac = std::array<std::byte, 32>;
using secret_key = std::array<std::byte, 32>;

// MUST override or use UDP_RELAY_SECRET_KEY env var
constexpr std::string_view handshake_secret_key = "fL6I8F3egv6ApC15fkJO9U7xKeDD6Xur";

// override if you feel like it or you want break compatability
constexpr uint32_t handshake_magic_number_host = 0x4B28000;
constexpr uint32_t handshake_magic_number_hton = ur::net::hton32(handshake_magic_number_host);

struct alignas(8) handshake_header
{
	uint32_t m_magicNumber{handshake_magic_number_host}; // relay packet identifier
	uint16_t m_length{};								 // support handhsake extensions
	uint16_t m_flags{};									 // support handhsake extensions
	guid m_guid{};										 // channel identifier
	uint64_t m_nonce{};									 // security nonce
	hmac m_mac{};										 // hmac
};
constexpr uint16_t handshake_min_size = 64;
static_assert(sizeof(handshake_header) == handshake_min_size);

class relay
{
public:
	using recv_buffer_t = std::array<std::uint64_t, 65536 / alignof(std::uint64_t)>;

	relay() = default;
	relay(const relay&) = delete;
	relay(relay&&) = delete;
	~relay() = default;

	// Initialize relay with params
	bool init(relay_params params, secret_key key);

	// Begin spin loop
	void run();

	// Immediate stop
	void stop();

	// Wait until all existing connections closed and then stop. Also prevents new connections being created.
	void stopGracefully();

private:
	void processIncoming();

	void conditionalCleanup();

	relay_params m_params{};

	secret_key m_secretKey{};

	udpsocket m_socket{};

	socket_address m_recvAddr{};

	recv_buffer_t m_recvBuffer{};

	std::unordered_map<guid, channel> m_channels{};

	std::unordered_map<socket_address, guid> m_addressChannels{};

	ur::ring_buffer<uint64_t, 32> m_recentNonces{};

	std::chrono::steady_clock::time_point m_lastTickTime{};

	std::chrono::steady_clock::time_point m_nextCleanupTime{};

	std::atomic_bool m_running{false};

	std::atomic_bool m_gracefulStopRequested{false};
};

struct relay_helpers
{
	static std::pair<bool, handshake_header> tryDeserializeHeader(const secret_key& key, const relay::recv_buffer_t& recvBuffer, size_t recvBytes);

	static secret_key makeSecret(std::string key);

	static hmac makeHMAC(const secret_key& key, uint64_t nonce);
};