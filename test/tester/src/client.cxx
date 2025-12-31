// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "client.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/relay.hxx"
#include "udp-relay/utils.hxx"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

std::pair<bool, relay_client_handshake> relay_client_helpers::tryDeserialize(const ur::secret_key& key, ur::recv_buffer& recvBuffer, size_t recvBytes)
{
	if (recvBytes < sizeof(relay_client_handshake))
		return std::pair<bool, relay_client_handshake>{};

	const auto [isHeader, header] = ur::relay_helpers::tryDeserializeHeader(key, recvBuffer, recvBytes);
	if (!isHeader)
		return std::pair<bool, relay_client_handshake>{};

	relay_client_handshake result{};
	std::memcpy(&result, recvBuffer.data(), sizeof(relay_client_handshake));

	result.m_header = header;
	result.m_type = ur::net::ntoh(result.m_type);
	result.m_time = ur::net::ntoh(result.m_time);

	return std::pair<bool, relay_client_handshake>(true, result);
}

bool relay_client::init(relay_client_params params, ur::secret_key key)
{
	auto socket = ur::net::udpsocket::make(params.m_relayAddr.isIpv6());
	if (!socket.isValid())
	{
		LOG(Error, RelayClient, "Failed to create socket");
		return false;
	}

	const bool useIpv6 = params.m_relayAddr.isIpv6();
	if (useIpv6 && !socket.setOnlyIpv6(false))
	{
		LOG(Error, RelayClient, "Failed set socket ipv6 to dual-stack mode");
		return false;
	}

	auto bindAddr = useIpv6 ? ur::net::socket_address::make_ipv6(ur::net::anyIpv6(), 0) : ur::net::socket_address::make_ipv4(ur::net::anyIpv4(), 0);
	if (!socket.bind(bindAddr))
	{
		LOG(Error, RelayClient, "Failed bind socket to {}", bindAddr);
		return false;
	}

	if (!socket.setNonBlocking(true))
	{
		LOG(Error, RelayClient, "Failed set non-blocking");
		return false;
	}

	m_params = std::move(params);
	m_secretKey = std::move(key);
	m_socket = std::move(socket);
	m_nonce = ur::randRange<uint64_t>(0, UINT64_MAX);

	return true;
}

void relay_client::run()
{
	if (!m_socket.isValid())
	{
		LOG(Error, RelayClient, "Attempt to run while not initialized");
		return;
	}

	m_running = true;
	m_allowSend = true;

	while (m_running)
	{
		if (m_socket.waitForRead(5000us))
			processIncoming();

		trySend();
	}
}

void relay_client::processIncoming()
{
	for (int32_t i = 0; i < 32; i++)
	{
		ur::net::socket_address recvAddr{};

		int32_t bytesRead{};
		bytesRead = m_socket.recvFrom(m_recvBuffer.data(), m_recvBuffer.size(), recvAddr);

		if (bytesRead < 0)
			return;

		const auto [packetOk, packet] = relay_client_helpers::tryDeserialize(m_secretKey, m_recvBuffer, bytesRead);
		if (!packetOk)
			continue;

		if (packet.m_header.m_guid != m_params.m_guid)
		{
			LOG(Warning, RelayClient, "Recv packet with invalid guid {} (expected: {})", packet.m_header.m_guid, m_params.m_guid);
			continue;
		}

		m_stats.m_packetsRecv++;

		// after relay established, it better to zero nonce to avoid unnecessary checks
		if (m_nonce)
			m_nonce = 0;

		if (packet.m_type == HandshakeRequest)
		{
			constexpr uint16_t responseType = ur::net::hton16(HandshakeResponse);
			std::memcpy(m_recvBuffer.data() + offsetof(relay_client_handshake, m_type), &responseType, sizeof(responseType));

			m_socket.waitForWrite(500us);
			const auto bytesSent = m_socket.sendTo(m_recvBuffer.data(), bytesRead, recvAddr);
			if (bytesSent >= 0)
				m_stats.m_packetsSent++;
		}
		else if (packet.m_type == HandshakeResponse)
		{
			const auto recvNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
			const auto sentNs = std::chrono::nanoseconds(packet.m_time);
			const auto packetTimeNs = recvNs - sentNs;
			const auto packetTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(packetTimeNs);
			m_stats.m_latenciesMs.push_back(packetTimeMs.count());
		}
		else
		{
			LOG(Warning, RelayClient, "Huh? unknown packet type {}", packet.m_type);
		}
	}
}

void relay_client::trySend()
{
	const auto now = std::chrono::steady_clock::now();
	if (m_allowSend && now - m_lastSendAt > m_params.m_sendIntervalMs)
	{
		m_lastSendAt = now;

		std::vector<std::byte> extraPayload;
		extraPayload.resize(ur::randRange<uint32_t>(0, 1472 - sizeof(relay_client_handshake)));
		std::generate(extraPayload.begin(), extraPayload.end(), []() -> std::byte
			{ return static_cast<std::byte>(ur::randRange<std::uint32_t>(0, UINT8_MAX)); });

		relay_client_handshake packet{};
		packet.m_header.m_length = ur::net::hton16(sizeof(relay_client_handshake) + extraPayload.size() - sizeof(relay_client_handshake::m_header));
		packet.m_header.m_guid = ur::net::hton(m_params.m_guid);
		packet.m_type = ur::net::hton16(HandshakeRequest);
		packet.m_time = ur::net::hton64(std::chrono::steady_clock::now().time_since_epoch().count());

		if (m_nonce)
		{
			packet.m_header.m_nonce = ur::net::hton64(m_nonce);
			packet.m_header.m_mac = ur::relay_helpers::makeHMAC(m_secretKey, packet.m_header.m_nonce);
		}

		std::vector<uint8_t> requestBytes;
		requestBytes.resize(sizeof(relay_client_handshake) + extraPayload.size());

		std::memcpy(requestBytes.data(), &packet, sizeof(relay_client_handshake));
		std::memcpy(requestBytes.data() + sizeof(relay_client_handshake), extraPayload.data(), extraPayload.size());

		const auto bytesSent = m_socket.sendTo(requestBytes.data(), requestBytes.size(), m_params.m_relayAddr);
		if (bytesSent >= 0)
			++m_stats.m_packetsSent;
	};
}

void relay_client::stopSending()
{
	m_allowSend = false;
}

void relay_client::stop()
{
	m_running = false;
}

int32_t relay_client::getMedianLatency() const
{
	if (m_stats.m_latenciesMs.size() <= 2)
		return -1;

	auto latencies = m_stats.m_latenciesMs;
	std::sort(latencies.begin(), latencies.end());

	const size_t median = latencies.size() / 2;
	return (latencies[median] + latencies[median - 1]) / 2;
}

int32_t relay_client::getAverageLatency() const
{
	if (m_stats.m_latenciesMs.size() == 0)
		return -1;

	int64_t sum{};
	for (auto latency : m_stats.m_latenciesMs)
	{
		sum += latency;
	}
	return sum / m_stats.m_latenciesMs.size();
}