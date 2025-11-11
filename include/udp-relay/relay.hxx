// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/networking/internetaddr.hxx"
#include "udp-relay/networking/network_utils.hxx"
#include "udp-relay/networking/udpsocket.hxx"

#include "types.hxx"

#include <array>
#include <chrono>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <queue>

using packet_buffer = std::array<uint8_t, 2048>;

struct channel_stats
{
	uint64_t m_bytesReceived{};
	uint64_t m_bytesSent{};

	uint32_t m_packetsReceived{};
	uint32_t m_packetsSent{};
};

struct channel
{
	channel_stats m_stats{};
	guid m_guid{};
	internetaddr m_peerA{};
	internetaddr m_peerB{};
	std::chrono::time_point<std::chrono::steady_clock> m_lastUpdated{};
};

struct pending_packet
{
	guid m_guid{};
	internetaddr m_target{};
	packet_buffer m_buffer{};
	int32_t m_bytesRead{};
};

struct relay_params
{
	uint16_t m_primaryPort{};
	int32_t m_warnTickExceedTimeUs{};
};

class relay
{
public:
	relay() = default;
	relay(const relay&) = delete;
	relay(relay&&) = delete;
	~relay() = default;

	bool run(const relay_params& params);

	void stop();

private:
	bool init();

	channel& createChannel(const guid& inGuid);

	void sendPendingPackets();

	void conditionalCleanup(bool force);

	void checkWarnLogTickTime();

	inline bool checkHandshakePacket(const packet_buffer& buffer, size_t bytesRead) const noexcept;

	relay_params m_params;

	std::map<guid, channel&> m_guidMappedChannels{};

	std::map<internetaddr, channel&> m_addressMappedChannels{};

	std::queue<pending_packet> m_pendingPackets{};

	std::list<channel> m_channels{};

	uniqueUdpsocket m_socket{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastCleanupTime{};

	bool m_running{false};
};

inline bool relay::checkHandshakePacket(const packet_buffer& buffer, const size_t bytesRead) const noexcept
{
	return bytesRead >= handshake_header_min_size;
}
