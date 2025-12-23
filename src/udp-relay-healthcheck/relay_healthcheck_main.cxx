// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/socket_address.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/relay.hxx"
#include "udp-relay/utils.hxx"

#include <array>
#include <chrono>
#include <cstdint>
#include <print>
#include <stacktrace>

using namespace std::chrono_literals;

namespace args
{
	bool printHelp{};
	std::string relayAddr{"127.0.0.1"};
	uint16_t relayPort{6060};
	uint16_t maxProbes{5};
	std::chrono::milliseconds waitTime{1s};
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	cl_arg_ref{"--help", args::printHelp,		"--help	= print help" },
	cl_arg_ref{"--addr", args::relayAddr,		"--addr <value>	= relay address" },
	cl_arg_ref{"--port", args::relayPort,		"--port 0-65535 = relay port" },
	cl_arg_ref{"--maxProbes", args::maxProbes,	"--maxProbes 0-65535 = maximum amount of tries before fail" },
	cl_arg_ref{"--waitTime", args::waitTime,	"--waitTime <value> = time in milliseconds to wait between attempts" },
};
// clang-format on

udpsocket make_socket(bool ipv6)
{
	auto socket = udpsocket::make(ipv6);
	const socket_address bindAddr = ipv6 ? socket_address::make_ipv6(ur::net::anyIpv6(), 0) : socket_address::make_ipv4(ur::net::anyIpv4(), 0);
	if (!socket.bind(bindAddr)) [[unlikely]]
		return udpsocket();
	if (!socket.setNonBlocking()) [[unlikely]]
		return udpsocket();
	return socket;
}

int relay_healthcheck_main(int argc, char* argv[], [[maybe_unused]] char* envp[])
{
	ur::parseArgs(argList, argc, argv);

	if (args::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	socket_address relayAddr = socket_address::from_string(args::relayAddr);
	relayAddr.setPort(args::relayPort);

	if (relayAddr.isNull() || !relayAddr.getPort())
	{
		std::println("Invalid address {}", relayAddr);
		return 1;
	}

	for (int32_t i = 0; i < args::maxProbes; ++i)
	{
		auto socketA = make_socket(relayAddr.isIpv6());
		auto socketB = make_socket(relayAddr.isIpv6());
		if (!socketA.isValid() || !socketB.isValid()) [[unlikely]]
		{
			std::println("Failed to make sockets");
			return 1;
		}

		handshake_header headerPacket{};
		headerPacket.m_magicNumber = handshake_magic_number_hton;
		headerPacket.m_guid = guid::newGuid();

		socketA.sendTo(&headerPacket, sizeof(headerPacket), relayAddr);
		socketB.sendTo(&headerPacket, sizeof(headerPacket), relayAddr);

		socket_address recvAddr{};
		relay::recv_buffer recvBuffer{};
		int32_t recvBytes{};

		const auto waitUntil = std::chrono::steady_clock::now() + args::waitTime;
		while (waitUntil > std::chrono::steady_clock::now())
		{
			if (socketA.waitForRead(args::waitTime / 2))
				recvBytes = socketA.recvFrom(recvBuffer.data(), recvBuffer.size(), recvAddr);
			else if (socketB.waitForRead(args::waitTime / 2))
				recvBytes = socketB.recvFrom(recvBuffer.data(), recvBuffer.size(), recvAddr);

			if (recvBytes > 0)
				break;
		}

		if (recvBytes <= 0)
		{
			std::println("Probe failed {} / {} (no data)", i + 1, args::maxProbes);
			continue;
		}
		else if (recvBytes != sizeof(handshake_header))
		{
			std::println("Probe failed {} / {} (unexpected size of {} instead {})", i + 1, args::maxProbes, recvBytes, sizeof(handshake_header));
			continue;
		}
		else if (recvAddr != relayAddr)
		{
			std::println("Huh? Received packet from address {}, when {} expected", recvAddr, relayAddr);
			// it's ok for the most part;
		}

		handshake_header recvHeader{};
		std::memcpy(&recvHeader, recvBuffer.data(), sizeof(recvHeader));

		if (recvHeader.m_magicNumber == headerPacket.m_magicNumber && recvHeader.m_guid == headerPacket.m_guid)
		{
			std::println("Probe succeeded! {} / {}", i + 1, args::maxProbes);
			return 0; // success!
		}
		else
		{
			std::println("Probe failed {} / {} (unexpected data)", i + 1, args::maxProbes);
			continue;
		}
	}

	return 0;
}