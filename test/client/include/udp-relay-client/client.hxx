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

private:
	bool init();

	relay_client_params m_params{};

	uniqueUdpsocket m_socket{};

	std::atomic_bool m_running = false;
};