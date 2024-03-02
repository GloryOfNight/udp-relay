#pragma once
#include "socket/internetaddr.hxx"
#include "socket/udpsocket.hxx"

#include "types.hxx"

#include <chrono>
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
	std::chrono::steady_clock::time_point m_lastUpdated{};
};

class relay
{
public:
	relay() = default;
	~relay() = default;

	bool run();

	void stop();

private:
	bool init();

	std::unique_ptr<udpsocket> m_socket{};

	std::chrono::steady_clock::time_point m_timePoint{};

	std::forward_list<channel> m_channels{};

	std::unordered_map<guid, const channel&> m_guidMappedChannels{};

	std::unordered_map<std::shared_ptr<internetaddr>, const channel&> m_addressMappedChannels{};

	bool m_running{false};
};