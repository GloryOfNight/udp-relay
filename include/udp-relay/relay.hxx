#pragma once
#include "udp-relay/internetaddr.hxx"
#include "udp-relay/udpsocket.hxx"

#include "types.hxx"

#include <array>
#include <chrono>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
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
	channel_stats m_stats{};
	guid m_guid{};
	internetaddr m_peerA{};
	internetaddr m_peerB{};
	std::chrono::time_point<std::chrono::steady_clock> m_lastUpdated{};
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

	bool conditionalCleanup(bool force);

	inline bool checkHandshakePacket(const std::array<uint8_t, 1024>& buffer, size_t bytesRead) const noexcept;

	relay_params m_params;

	std::unordered_map<guid, channel&> m_guidMappedChannels{};

	std::unordered_map<internetaddr, channel&> m_addressMappedChannels{};

	std::list<channel> m_channels{};

	uniqueUdpsocket m_socket{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastCleanupTime{};

	bool m_running{false};
};

inline bool relay::checkHandshakePacket(const std::array<uint8_t, 1024>& buffer, const size_t bytesRead) const noexcept
{
	const handshake_header* header = reinterpret_cast<const handshake_header*>(buffer.data());
	return bytesRead == 1024 && BYTESWAP16(header->m_type) == 1 && BYTESWAP16(header->m_length) == 992;
}