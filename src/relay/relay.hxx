#pragma once
#include "socket/internetaddr.hxx"
#include "socket/udpsocket.hxx"

#include "types.hxx"

#include <chrono>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>

struct channel_stats
{
	uint32_t m_bytesReceived{};
	uint32_t m_packetsReceived{};

	uint32_t m_bytesSent{};
	uint32_t m_packetsSent{};

	uint32_t m_bytesDropped{};
	uint32_t m_packetsDropped{};
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
	~relay() = default;

	bool run();

	void stop();

private:
	bool init();

	channel& createChannel(const guid& inGuid);

	bool conditionalCleanup(bool force);

	std::unordered_map<guid, channel&> m_guidMappedChannels{};

	std::unordered_map<std::shared_ptr<internetaddr>, channel&> m_addressMappedChannels{};

	std::list<channel> m_channels{};

	std::unique_ptr<udpsocket> m_socket{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastCleanupTime{};

	bool m_running{false};
};