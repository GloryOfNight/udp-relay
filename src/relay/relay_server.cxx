// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay/relay_server.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/internetaddr.hxx"
#include "udp-relay/relay/relay_consts.hxx"

#include <array>
#include <thread>

relay_server::relay_server(const relay_server_params& params)
{
	m_params = params;
	m_lastTickTime = std::chrono::steady_clock::now();
}

void relay_server::start()
{
	LOG(Info, Relay, "Begin relay initialization. Port: {}. Worked port range {}-{} (external {}-{})", m_params.m_primaryPort, m_params.m_workerPortStart, m_params.m_workerPortEnd, m_params.m_workerExternalPortStart, m_params.m_workerExternalPortEnd);

	m_socket = udpsocketFactory::createUdpSocket();
	if (m_socket == nullptr || !m_socket->isValid())
	{
		LOG(Error, Relay, "Failed to create socket!");
		return;
	}

	if (!m_socket->bind(m_params.m_primaryPort))
	{
		LOG(Error, Relay, "Failed bind socket to {0} port", m_params.m_primaryPort);
		return;
	}

	if (!m_socket->setSendTimeoutUs(ur::consts::sendTimeoutUs))
	{
		LOG(Error, Relay, "Failed set socket send timeout");
		return;
	}

	if (!m_socket->setRecvTimeoutUs(ur::consts::recvTimeoutUs))
	{
		LOG(Error, Relay, "Failed set socket recv timeout");
		return;
	}

	int32_t actualSendBufferSize{};
	if (!m_socket->setSendBufferSize(ur::consts::desiredSendBufferSize, actualSendBufferSize))
		LOG(Error, Relay, "Error with setting send buffer size");

	int32_t actualRecvBufferSize{};
	if (!m_socket->setRecvBufferSize(ur::consts::desiredRecvBufferSize, actualRecvBufferSize))
		LOG(Error, Relay, "Error with setting recv buffer size");

	LOG(Info, Relay, "Relay main socket initialized on port: {}. SendBufSize: {}. RecvBufSize: {}.", m_socket->getPort(), actualSendBufferSize, actualRecvBufferSize);

	m_workers.reserve(m_params.m_workerPortEnd - m_params.m_workerPortStart);
	for (uint16_t i = m_params.m_workerPortStart; i < m_params.m_workerPortEnd; ++i)
	{
		relay_worker_params workerParams{};
		workerParams.workerId = i - m_params.m_workerPortStart;
		workerParams.port = i;

		auto& newRelayWorker = m_workers.emplace_back(std::make_unique<relay_worker>(workerParams));
		m_worker_threads.emplace_back(std::jthread(&relay_worker::start, newRelayWorker.get()));
	}

	m_running = true;
	while (m_running)
	{
		update();
	}
}

void relay_server::stop()
{
	LOG(Info, Relay, "Relay stop requested. Stopping all.");

	m_running = false;
	for (auto& worker : m_workers)
	{
		worker->stop();
	}
}

void relay_server::update()
{
	std::chrono::time_point<std::chrono::steady_clock> prevTickTime = m_lastTickTime;
	m_lastTickTime = std::chrono::steady_clock::now();

	if (!m_socket->waitForReadUs(1'000'000))
		return;

	std::array<uint8_t, 2048> buffer{};
	internetaddr recvAddr{};

	while (m_socket->recvFrom(buffer.data(), buffer.size(), &recvAddr))
	{
	}
}
