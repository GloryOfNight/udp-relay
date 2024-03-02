#pragma once
#include "socket/internetaddr.hxx"
#include "socket/udpsocket.hxx"

#include "types.hxx"

#include <cstdint>
#include <forward_list>
#include <map>
#include <memory>
#include <unordered_map>

struct channel
{
	guid m_guid{};
	std::shared_ptr<internetaddr> m_peerA{};
	std::shared_ptr<internetaddr> m_peerB{};
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

	std::unique_ptr<udpsocket> m_socket{};

	std::forward_list<channel> m_channels{};

	std::unordered_map<guid, channel&> m_guidMappedChannels{};

	std::unordered_map<std::shared_ptr<internetaddr>, const channel&> m_addressMappedChannels{};

	bool m_running{false};
};