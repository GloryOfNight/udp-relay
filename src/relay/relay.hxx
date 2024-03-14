#pragma once
#include "socket/internetaddr.hxx"
#include "socket/udpsocket.hxx"

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
	std::shared_ptr<internetaddr> m_peerA{};
	std::shared_ptr<internetaddr> m_peerB{};
	std::chrono::time_point<std::chrono::steady_clock> m_lastUpdated{};
};

#ifdef _MSC_VER
#define NETWORK_TO_HOST_16(x) _byteswap_ushort(static_cast<uint16_t>(x))
#define NETWORK_TO_HOST_32(x) _byteswap_ulong(static_cast<uint32_t>(x))
#elif defined(__clang__)
#define NETWORK_TO_HOST_16(x) __builtin_bswap16(static_cast<uint16_t>(x))
#define NETWORK_TO_HOST_32(x) __builtin_bswap32(static_cast<uint32_t>(x))
#endif

class relay
{
public:
	relay() = default;
	relay(const relay&) = delete;
	relay(relay&&) = delete;
	~relay() = default;

	bool run(const uint16_t port);

	void stop();

private:
	bool init(const uint16_t port);

	channel& createChannel(const guid& inGuid);

	bool conditionalCleanup(bool force);

	inline bool checkHandshakePacket(const std::array<uint8_t, 1024>& buffer, size_t bytesRead) const noexcept;

	std::unordered_map<guid, channel&> m_guidMappedChannels{};

	std::unordered_map<sharedInternetaddr, channel&> m_addressMappedChannels{};

	std::list<channel> m_channels{};

	uniqueUdpsocket m_socket{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastCleanupTime{};

	bool m_running{false};
};

inline bool relay::checkHandshakePacket(const std::array<uint8_t, 1024>& buffer, const size_t bytesRead) const noexcept
{
	const handshake_header* header = reinterpret_cast<const handshake_header*>(buffer.data());
	return bytesRead == 1024 && NETWORK_TO_HOST_16(header->m_type) == 1 && NETWORK_TO_HOST_16(header->m_length) == 992;
}