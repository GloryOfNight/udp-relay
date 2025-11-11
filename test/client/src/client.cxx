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
	int64_t m_time{};
	std::array<uint8_t, 992> m_randomData{};

	static handshake_packet newPacket(const guid& guid)
	{
		handshake_packet packet{};

		packet.m_header.m_type = ur::net::hton16(1);
		packet.m_header.m_length = ur::net::hton16(992);
		packet.m_header.m_guid.m_a = ur::net::hton32(guid.m_a);
		packet.m_header.m_guid.m_b = ur::net::hton32(guid.m_b);
		packet.m_header.m_guid.m_c = ur::net::hton32(guid.m_c);
		packet.m_header.m_guid.m_d = ur::net::hton32(guid.m_d);

		packet.m_time = ur::net::hton64(std::chrono::steady_clock::now().time_since_epoch().count());

		return packet;
	}

	void generateRandomPayload()
	{
		// fill array with random bytes
		std::generate(m_randomData.begin(), m_randomData.end(), []() -> uint8_t
			{ return ur::randRange<uint32_t>(0, UINT8_MAX); });
	}
};

void relay_client::run(const relay_client_params& params)
{
	m_params = params;

	if (!init())
	{
		LOG(Error, RelayClient, "Client failed to initialize");
		return;
	}

	std::array<uint8_t, 1024> buffer;

	internetaddr recvAddr{};

	internetaddr relayAddr{};
	relayAddr.setIp(m_params.m_server_ip);
	relayAddr.setPort(htons(m_params.m_server_port));

	auto lastSendTime = std::chrono::steady_clock::now();

	m_running = true;

	bool relayEstablished = false;

	while (m_running)
	{
		if (m_socket->waitForReadUs(0))
		{
			int32_t bytesRead{};

			relayEstablished = true;
			bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), &recvAddr);

			if (bytesRead >= sizeof(handshake_header))
			{
				++m_packetsRecv;

				const auto* packet = reinterpret_cast<handshake_packet*>(buffer.data());
				const auto* header = &packet->m_header;

				if (header->m_type == ur::net::hton16(1))
				{
					handshake_packet recvPacket = reinterpret_cast<handshake_packet&>(*buffer.data());
					recvPacket.m_header.m_type = ur::net::hton16(2);

					const auto bytesSent = m_socket->sendTo(&recvPacket, bytesRead, &relayAddr);
					if (bytesSent > 0)
						++m_packetsSent;
				}
				else if (header->m_type == ur::net::hton16(2))
				{
					const auto recvNs = std::chrono::steady_clock::now().time_since_epoch().count();
					const auto sentNs = ur::net::ntoh64(packet->m_time);
					const auto packetTimeNs = std::chrono::nanoseconds(recvNs - sentNs);
					const auto packetTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(packetTimeNs);
					m_latenciesMs.push_back(packetTimeMs.count());
				}
			}
		}

		const auto now = std::chrono::steady_clock::now();
		if (now - lastSendTime > std::chrono::milliseconds(m_params.m_sendIntervalMs))
		{
			lastSendTime = now;

			handshake_packet packet = handshake_packet::newPacket(m_params.m_guid);
			packet.generateRandomPayload();

			const auto randomOffset = relayEstablished ? ur::randRange<uint32_t>(sizeof(handshake_header), packet.m_randomData.size() - 1) : 0;

			if (m_socket->waitForReadUs(0))
				continue;

			const auto bytesSent = m_socket->sendTo(&packet, sizeof(packet) - randomOffset, &relayAddr);
			if (bytesSent > 0)
				++m_packetsSent;
		};

		std::this_thread::sleep_for(std::chrono::milliseconds(m_params.m_sleepMs));
	}
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