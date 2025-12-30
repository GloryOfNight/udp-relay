// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay-client/client.hxx"

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

const uint32_t handshake_packet_data_size = (sizeof(relay_client_handshake) - sizeof(relay_client_handshake::m_randomPayload));

std::vector<uint8_t> relay_client_helpers::serialize(const relay_client_handshake& value)
{
	relay_client_handshake netValue{};

	netValue.m_header.m_magicNumber = ur::net::hton32(value.m_header.m_magicNumber);
	netValue.m_header.m_guid.m_a = ur::net::hton32(value.m_header.m_guid.m_a);
	netValue.m_header.m_guid.m_b = ur::net::hton32(value.m_header.m_guid.m_b);
	netValue.m_header.m_guid.m_c = ur::net::hton32(value.m_header.m_guid.m_c);
	netValue.m_header.m_guid.m_d = ur::net::hton32(value.m_header.m_guid.m_d);
	netValue.m_header.m_nonce = value.m_header.m_nonce;
	netValue.m_header.m_mac = value.m_header.m_mac;
	netValue.m_type = ur::net::hton16(value.m_type);
	netValue.m_time = ur::net::hton64(value.m_time);
	netValue.m_randomPayload = value.m_randomPayload;

	std::vector<uint8_t> output{};
	output.resize(sizeof(relay_client_handshake));
	std::memcpy(output.data(), &netValue, sizeof(netValue));

	return output;
}

std::pair<bool, relay_client_handshake> relay_client_helpers::tryDeserialize(const secret_key& key, relay::recv_buffer_t& recvBuffer, size_t recvBytes)
{
	if (recvBytes < handshake_packet_data_size)
		return std::pair<bool, relay_client_handshake>{};

	const auto [isHeader, header] = relay_helpers::tryDeserializeHeader(key, recvBuffer, recvBytes);
	if (!isHeader)
		return std::pair<bool, relay_client_handshake>{};

	relay_client_handshake result{};
	std::memcpy(&result, recvBuffer.data(), handshake_packet_data_size);

	result.m_header = header;
	result.m_type = ur::net::ntoh16(result.m_type);
	result.m_time = ur::net::ntoh64(result.m_time);

	return std::pair<bool, relay_client_handshake>(true, result);
}

bool relay_client::init(relay_client_params params, secret_key key)
{
	auto socket = udpsocket::make(params.m_relayAddr.isIpv6());
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

	auto bindAddr = useIpv6 ? socket_address::make_ipv6(ur::net::anyIpv6(), 0) : socket_address::make_ipv4(ur::net::anyIpv4(), 0);
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
		socket_address recvAddr{};

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

		if (packet.m_type == HandshakeRequest)
		{
			auto responsePacket = packet;
			responsePacket.m_header.m_magicNumber = handshake_magic_number_host;
			responsePacket.m_type = HandshakeResponse;

			auto responseBuf = relay_client_helpers::serialize(responsePacket);

			m_socket.waitForWrite(500us);
			const auto bytesSent = m_socket.sendTo(responseBuf.data(), bytesRead, recvAddr);
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

		relay_client_handshake packet{};
		packet.m_header.m_guid = m_params.m_guid;
		packet.m_header.m_nonce = ur::net::hton64(m_nonce);
		packet.m_header.m_mac = relay_helpers::makeHMAC(m_secretKey, packet.m_header.m_nonce);
		packet.m_type = HandshakeRequest;
		packet.m_time = std::chrono::steady_clock::now().time_since_epoch().count();
		packet.generateRandomPayload();

		std::vector<uint8_t> requestBuf = relay_client_helpers::serialize(packet);

		const uint32_t payloadStripOffset = ur::randRange<uint32_t>(0, requestBuf.size() - handshake_packet_data_size);

		const auto bytesSent = m_socket.sendTo(requestBuf.data(), requestBuf.size() - payloadStripOffset, m_params.m_relayAddr);
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