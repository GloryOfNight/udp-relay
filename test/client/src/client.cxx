#include "udp-relay-client/client.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/udpsocketFactory.hxx"
#include "udp-relay/utils.hxx"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

struct handshake_packet
{
	handshake_header m_header{};
	std::array<uint8_t, 1024 - sizeof(handshake_header)> m_randomData{};

	static handshake_packet newPacket(const guid& guid)
	{
		handshake_packet packet{};

		packet.m_header.m_type = htons(1);
		packet.m_header.m_length = htons(992);
		packet.m_header.m_guid.m_a = htonl(guid.m_a);
		packet.m_header.m_guid.m_b = htonl(guid.m_b);
		packet.m_header.m_guid.m_c = htonl(guid.m_c);
		packet.m_header.m_guid.m_d = htonl(guid.m_d);

		return packet;
	}

	void generateRandomPayload()
	{
		// fill array with random bytes
		std::generate(m_randomData.begin(), m_randomData.end(), []() -> uint8_t
			{ return udprelay::utils::randRange<uint32_t>(0, UINT8_MAX); });
	}
};

void relay_client::run(const relay_client_params& params)
{
	m_params = params;

	if (!init())
	{
		LOG(Display, "Client failed to initialize");
		return;
	}

	LOG(Display, "Client initialized on {0} port", m_socket->getPort());
	std::array<uint8_t, 1024> buffer;

	sharedInternetaddr recvAddr = udpsocketFactory::createInternetAddr();

	const uniqueInternetaddr relay_addr = udpsocketFactory::createInternetAddrUnique();
	relay_addr->setIp(m_params.m_server_ip);
	relay_addr->setPort(htons(m_params.m_server_port));

	auto lastSendTime = std::chrono::steady_clock::now();

	LOG(Display, "Client attempt send packets to {0}", relay_addr->toString());

	m_running = true;
	while (m_running)
	{
		if (m_socket->waitForRead(100))
		{
			int32_t bytesRead{};
			do
			{
				bytesRead = m_socket->recvFrom(buffer.data(), buffer.size(), recvAddr.get());
			} while (bytesRead > 0);
		}

		const auto now = std::chrono::steady_clock::now();
		if (now - lastSendTime > std::chrono::milliseconds(m_params.m_sendIntervalMs))
		{
			lastSendTime = now;

			handshake_packet packet = handshake_packet::newPacket(m_params.m_guid);
			packet.generateRandomPayload();

			const auto bytesSent = m_socket->sendTo(&packet, sizeof(packet), relay_addr.get());
		};

		std::this_thread::sleep_for(std::chrono::milliseconds(m_params.m_sleepMs));
	}
}

void relay_client::stop()
{
	m_running = false;
}

bool relay_client::init()
{
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