// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/networking/internetaddr.hxx"
#include "udp-relay/networking/udpsocket.hxx"

#include "relay_types.hxx"
#include "relay_worker.hxx"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <string_view>
#include <thread>
#include <vector>

struct relay_server_params
{
	uint16_t m_primaryPort{6060};

	uint16_t m_workerPortStart{6061};
	uint16_t m_workerPortEnd{6068};

	uint16_t m_workerExternalPortStart{m_workerPortStart};
	uint16_t m_workerExternalPortEnd{m_workerPortEnd};

	uint32_t m_publicPassword{123};
	uint32_t m_privatePassword{0x120597};
	uint32_t m_secretKey1{0xF2345678};
	uint32_t m_secretKey2{0x8765432F};

	uint32_t m_challengeTimeoutMs{5000};
};

struct relay_server_challenge
{
	internetaddr m_addr{};
	uint32_t m_secret{};
	std::chrono::time_point<std::chrono::steady_clock> m_sendTime{};
};

//
class relay_server final
{
	// maximum possible payload for udp packet
	using udp_buffer_t = std::array<uint8_t, 65507>;

public:
	relay_server() = default;
	relay_server(const relay_server_params& params);

	void start();
	void stop();

private:
	void update();

	void processCreateAllocationRequest();

	void challengeResponse();

	void createAllocationResponse();

	void errorResponse(ur::errorType errorType, std::string_view message);

	template <typename T>
	void prepareResponseHeader(T& buffer, ur::packetType type, uint16_t length);

	relay_server_params m_params{};

	uniqueUdpsocket m_socket{};

	std::vector<uniqueRelayWorker> m_workers{};
	std::vector<std::jthread> m_worker_threads{};

	internetaddr m_recv_addr{};
	udp_buffer_t m_recv_buffer{};
	int32_t m_recv_bytes{};

	std::array<relay_server_challenge, 32> m_challenges{};

	std::atomic_bool m_running{};
};

using uniqueRelayServer = std::unique_ptr<relay_server>;

template <typename T>
inline void relay_server::prepareResponseHeader(T& buffer, ur::packetType type, uint16_t length)
{
	uint16_t* typePtr = reinterpret_cast<uint16_t*>(buffer[0]);
	*typePtr = ur::hton16(static_cast<uint16_t>(type));

	uint16_t* lengthPtr = reinterpret_cast<uint16_t*>(buffer[2]);
	*lengthPtr = ur::hton16(length);

	std::memcpy(buffer.data() + 4, m_recv_buffer.data() + 4, 20);
}
