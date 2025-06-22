// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/networking/udpsocket.hxx"

#include <atomic>
#include <chrono>
#include <memory>

struct relay_worker_params
{
	uint16_t workerId{};
	uint16_t port{};
};

//
class relay_worker
{
public:
	relay_worker() = default;
	relay_worker(const relay_worker_params& params);

	void start();
	void stop();

private:
	void update();

	relay_worker_params m_params{};

	uniqueUdpsocket m_socket{};

	std::chrono::time_point<std::chrono::steady_clock> m_lastTickTime{};

	std::atomic_bool m_running{};
};

using uniqueRelayWorker = std::unique_ptr<relay_worker>;