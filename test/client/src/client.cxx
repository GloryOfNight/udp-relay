// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay-client/client.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/network_utils.hxx"
#include "udp-relay/networking/udpsocket.hxx"
#include "udp-relay/utils.hxx"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

struct handshake_packet
{
	handshake_header m_header{};
	uint16_t m_type{};
	int64_t m_time{};
	std::array<uint8_t, 992> m_randomData{};

	void generateRandomPayload()
	{
		// fill array with random bytes
		std::generate(m_randomData.begin(), m_randomData.end(), []() -> uint8_t
			{ return ur::randRange<uint32_t>(0, UINT8_MAX); });
	}
};

const uint32_t handshake_packet_data_size = (sizeof(handshake_packet) - sizeof(handshake_packet::m_randomData));

static std::vector<uint8_t> serializePacket(const handshake_packet& value)
{
	handshake_packet netValue{};

	netValue.m_header.m_magicNumber = ur::net::hton32(value.m_header.m_magicNumber);
	netValue.m_header.m_guid.m_a = ur::net::hton32(value.m_header.m_guid.m_a);
	netValue.m_header.m_guid.m_b = ur::net::hton32(value.m_header.m_guid.m_b);
	netValue.m_header.m_guid.m_c = ur::net::hton32(value.m_header.m_guid.m_c);
	netValue.m_header.m_guid.m_d = ur::net::hton32(value.m_header.m_guid.m_d);
	netValue.m_type = ur::net::hton16(value.m_type);
	netValue.m_time = ur::net::hton64(value.m_time);
	netValue.m_randomData = value.m_randomData;

	std::vector<uint8_t> output{};
	output.resize(sizeof(handshake_packet));
	std::memcpy(output.data(), &netValue, sizeof(netValue));
	return output;
}

static std::pair<bool, handshake_packet> deserializePacket(const uint8_t* buf, const uint32_t len)
{
	if (len < handshake_packet_data_size)
		return std::pair<bool, handshake_packet>{};

	handshake_packet netValue{};
	std::memcpy(&netValue, buf, handshake_packet_data_size);

	handshake_packet hostValue{};
	hostValue.m_header.m_magicNumber = ur::net::ntoh32(netValue.m_header.m_magicNumber);
	hostValue.m_header.m_guid.m_a = ur::net::ntoh32(netValue.m_header.m_guid.m_a);
	hostValue.m_header.m_guid.m_b = ur::net::ntoh32(netValue.m_header.m_guid.m_b);
	hostValue.m_header.m_guid.m_c = ur::net::ntoh32(netValue.m_header.m_guid.m_c);
	hostValue.m_header.m_guid.m_d = ur::net::ntoh32(netValue.m_header.m_guid.m_d);
	hostValue.m_type = ur::net::ntoh16(netValue.m_type);
	hostValue.m_time = ur::net::ntoh64(netValue.m_time);

	return std::pair<bool, handshake_packet>(true, hostValue);
}

void relay_client::run(const relay_client_params& params)
{
	m_params = params;

	if (!init())
	{
		LOG(Error, RelayClient, "Client failed to initialize");
		return;
	}

	std::vector<uint8_t> buffer;
	buffer.resize(2048);

	internetaddr recvAddr{};

	internetaddr relayAddr{};
	relayAddr.setIp(m_params.m_server_ip);
	relayAddr.setPort(htons(m_params.m_server_port));

	auto lastSendTime = std::chrono::steady_clock::now();

	m_running = true;
	m_allowSend = true;

	bool relayEstablished = false;

	while (m_running)
	{
		if (m_socket->waitForReadUs(500))
		{
			int32_t bytesRead{};

			relayEstablished = true;
			bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), &recvAddr);

			if (bytesRead >= 0)
			{
				const auto [packetOk, packet] = deserializePacket(buffer.data(), bytesRead);
				if (!packetOk)
					continue;

				++m_packetsRecv;

				if (packet.m_type == 1)
				{
					auto responsePacket = packet;
					responsePacket.m_type = 2;

					auto responseBuf = serializePacket(responsePacket);

					m_socket->waitForWriteUs(500);
					const auto bytesSent = m_socket->sendTo(responseBuf.data(), bytesRead, &relayAddr);
					if (bytesSent >= 0)
						++m_packetsSent;
				}
				else if (packet.m_type == 2)
				{
					const auto recvNs = std::chrono::steady_clock::now().time_since_epoch().count();
					const auto sentNs = packet.m_time;
					const auto packetTimeNs = std::chrono::nanoseconds(recvNs - sentNs);
					const auto packetTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(packetTimeNs);
					m_latenciesMs.push_back(packetTimeMs.count());
				}
				else
				{
					LOG(Warning, RelayClient, "Huh? unknown packet type {}", packet.m_type);
				}
			}
		}

		const auto now = std::chrono::steady_clock::now();
		if (m_allowSend && now - lastSendTime > std::chrono::milliseconds(m_params.m_sendIntervalMs))
		{
			lastSendTime = now;

			handshake_packet packet{};
			packet.m_header.m_guid = m_params.m_guid;
			packet.m_type = 1;
			packet.m_time = std::chrono::steady_clock::now().time_since_epoch().count();
			packet.generateRandomPayload();

			std::vector<uint8_t> requestBuf = serializePacket(packet);

			const uint32_t payloadStripOffset = relayEstablished ? ur::randRange<uint32_t>(0, requestBuf.size() - handshake_packet_data_size) : 0;

			m_socket->waitForWriteUs(500);
			const auto bytesSent = m_socket->sendTo(requestBuf.data(), requestBuf.size() - payloadStripOffset, &relayAddr);
			if (bytesSent >= 0)
				++m_packetsSent;
		};
	}
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
	if (m_latenciesMs.size() <= 2)
		return -1;

	auto latencies = m_latenciesMs;
	std::sort(latencies.begin(), latencies.end());

	const size_t median = latencies.size() / 2;
	return (latencies[median] + latencies[median - 1]) / 2;
}

int32_t relay_client::getAverageLatency() const
{
	if (m_latenciesMs.size() == 0)
		return -1;

	int64_t sum{};
	for (auto latency : m_latenciesMs)
	{
		sum += latency;
	}
	return sum / m_latenciesMs.size();
}

bool relay_client::init()
{
	m_latenciesMs.reserve(2048);

	m_socket = udpsocketFactory::createUdpSocket();
	if (!m_socket || !m_socket->isValid())
	{
		return false;
	}

	if (!m_socket->setNonBlocking(true))
	{
		return false;
	}

	if (!m_socket->bind(0))
	{
		return false;
	}

	return true;
}