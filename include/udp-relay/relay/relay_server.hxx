// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/networking/udpsocket.hxx"

#include "relay_worker.hxx"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

struct relay_server_params
{
	uint16_t m_primaryPort{6060};

	uint16_t m_workerPortStart{6061};
	uint16_t m_workerPortEnd{6068};

	uint16_t m_workerExternalPortStart{m_workerPortStart};
	uint16_t m_workerExternalPortEnd{m_workerPortEnd};
};

//
class relay_server final
{
public:
	relay_server() = default;
	relay_server(const relay_server_params& params);

	void start();
	void stop();

private:
	void update();

	relay_server_params m_params{};

	uniqueUdpsocket m_socket{};

	std::vector<uniqueRelayWorker> m_workers{};
	std::vector<std::jthread> m_worker_threads{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::atomic_bool m_running{};
};

using uniqueRelayServer = std::unique_ptr<relay_server>;