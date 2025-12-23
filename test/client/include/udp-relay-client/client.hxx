// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/guid.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/relay.hxx"

#include <array>
#include <atomic>

struct relay_client_params
{
	guid m_guid{};
	std::chrono::milliseconds m_sendIntervalMs{};
	socket_address m_relayAddr{};
};

struct relay_client_stats
{
	std::vector<int32_t> m_latenciesMs{};
	uint32_t m_packetsSent = 0;
	uint32_t m_packetsRecv = 0;
};

enum
{
	HandshakeRequest = 1,
	HandshakeResponse = 2
};

struct relay_client_handshake
{
	handshake_header m_header{};
	uint16_t m_type{};
	int64_t m_time{};
	std::array<uint8_t, 992> m_randomPayload{};

	void generateRandomPayload()
	{
		// fill array with random bytes
		std::generate(m_randomPayload.begin(), m_randomPayload.end(), []() -> uint8_t
			{ return ur::randRange<uint32_t>(0, UINT8_MAX); });
	}
};

class relay_client
{
public:
	bool init(relay_client_params params);

	void run();

	void stopSending();
	void stop();

	int32_t getMedianLatency() const;
	int32_t getAverageLatency() const;

	int32_t getPacketsSent() const { return m_stats.m_packetsSent; };
	int32_t getPacketsRecv() const { return m_stats.m_packetsRecv; };

	guid getGuid() const { return m_params.m_guid; };

	void processIncoming();

	void trySend();

private:
	relay_client_params m_params{};

	udpsocket m_socket{};

	relay::recv_buffer m_recvBuffer{};

	std::chrono::steady_clock::time_point m_lastSendAt{};

	relay_client_stats m_stats{};

	std::atomic_bool m_running = false;
	std::atomic_bool m_allowSend = false;
};

struct relay_client_helpers
{
	static std::vector<uint8_t> serialize(const relay_client_handshake& value);
	static std::pair<bool, relay_client_handshake> tryDeserialize(relay::recv_buffer& recvBuffer, size_t recvBytes);
};