// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay/relay_worker.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/internetaddr.hxx"
#include "udp-relay/relay/relay_consts.hxx"

#include <array>

relay_worker::relay_worker(const relay_worker_params& params)
{
	m_params = params;
}

void relay_worker::start()
{
	m_socket = udpsocketFactory::createUdpSocket();
	if (m_socket == nullptr || !m_socket->isValid())
	{
		LOG(Fatal, RelayWorker, "Failed to create socket!");
		return;
	}

	if (!m_socket->bind(m_params.port))
	{
		LOG(Fatal, RelayWorker, "Failed bind socket to {0} port", m_params.port);
		return;
	}

	if (!m_socket->setSendTimeoutUs(ur::consts::sendTimeoutUs))
	{
		LOG(Fatal, RelayWorker, "Failed set socket send timeout");
		return;
	}

	if (!m_socket->setRecvTimeoutUs(ur::consts::recvTimeoutUs))
	{
		LOG(Fatal, RelayWorker, "Failed set socket recv timeout");
		return;
	}

	int32_t actualSendBufferSize{};
	if (!m_socket->setSendBufferSize(ur::consts::desiredSendBufferSize, actualSendBufferSize))
		LOG(Error, RelayWorker, "Error with setting send buffer size");

	int32_t actualRecvBufferSize{};
	if (!m_socket->setRecvBufferSize(ur::consts::desiredRecvBufferSize, actualRecvBufferSize))
		LOG(Error, RelayWorker, "Error with setting recv buffer size");

	LOG(Info, RelayWorker, "Worker {} initialized on port: {}. SendBufSize: {}. RecvBufSize: {}.", m_params.workerId, m_socket->getPort(), actualSendBufferSize, actualRecvBufferSize);

	m_running = true;
	while (m_running)
	{
		update();
	}
}

void relay_worker::stop()
{
	m_running = false;
}

void relay_worker::update()
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
