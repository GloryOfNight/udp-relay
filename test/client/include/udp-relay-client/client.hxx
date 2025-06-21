// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/types.hxx"
#include "udp-relay/udpsocket.hxx"

#include <atomic>

struct relay_client_params
{
	guid m_guid{};
	int32_t m_sendIntervalMs{};
	int32_t m_sleepMs{};
	int32_t m_server_ip{};
	uint16_t m_server_port{};
};

class relay_client
{
public:
	void run(const relay_client_params& params);
	void stop();

	int32_t getMedianLatency() const;
	int32_t getAverageLatency() const;
	int32_t getPacketsSent() const { return m_packetsSent; };
	int32_t getPacketsRecv() const { return m_packetsRecv; };
	guid getGuid() const { return m_params.m_guid; };

private:
	bool init();

	relay_client_params m_params{};

	uniqueUdpsocket m_socket{};

	std::vector<int32_t> m_latencies{};

	uint32_t m_packetsSent = 0;
	uint32_t m_packetsRecv = 0;

	std::atomic_bool m_running = false;
};