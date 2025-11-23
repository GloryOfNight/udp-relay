// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/guid.hxx"
#include "udp-relay/net/udpsocket.hxx"

#include <atomic>

struct relay_client_params
{
	guid m_guid{};
	int32_t m_sendIntervalMs{};
	int32_t m_server_ip{};
	uint16_t m_server_port{};
	bool useIpv6{};
};

class relay_client
{
public:
	void run(const relay_client_params& params);
	void stopSending();
	void stop();

	int32_t getMedianLatency() const;
	int32_t getAverageLatency() const;
	int32_t getPacketsSent() const { return m_packetsSent; };
	int32_t getPacketsRecv() const { return m_packetsRecv; };
	guid getGuid() const { return m_params.m_guid; };

	void processIncoming();

	void trySend();

private:
	bool init();

	relay_client_params m_params{};

	uniqueUdpsocket m_socket{};

	std::vector<uint8_t> m_recvBuffer{};

	std::vector<int32_t> m_latenciesMs{};

	std::chrono::steady_clock::time_point m_lastSendAt{};

	uint32_t m_packetsSent = 0;
	uint32_t m_packetsRecv = 0;

	std::atomic_bool m_running = false;
	std::atomic_bool m_allowSend = false;
};