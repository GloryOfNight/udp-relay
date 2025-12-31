// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/main_helpers.hxx"
#include "udp-relay/net/network_utils.hxx"
#include "udp-relay/net/socket_address.hxx"
#include "udp-relay/net/udpsocket.hxx"
#include "udp-relay/relay.hxx"

#include <array>
#include <chrono>
#include <cstdint>
#include <print>
#include <stacktrace>

using namespace std::chrono_literals;

namespace cl
{
	static bool printHelp{};
	static std::string relayAddr{"127.0.0.1"};
	static uint16_t relayPort{6060};
	static uint16_t maxProbes{5};
	static std::chrono::milliseconds waitTime{1s};
} // namespace cl

namespace env
{
	static std::string_view secretKey{};
} // namespace env

// clang-format off
static constexpr auto argList = std::array
{
	ur::cl_var_ref{"--help", cl::printHelp,		"--help	= print help" },
	ur::cl_var_ref{"--addr", cl::relayAddr,		"--addr <value>	= relay address" },
	ur::cl_var_ref{"--port", cl::relayPort,		"--port 0-65535 = relay port" },
	ur::cl_var_ref{"--maxProbes", cl::maxProbes,"--maxProbes 0-65535 = maximum amount of tries before fail" },
	ur::cl_var_ref{"--waitTime", cl::waitTime,	"--waitTime <value> = time in milliseconds to wait between attempts" },
};

static constexpr auto envList = std::array
{
	ur::env_var_ref{"UDP_RELAY_SECRET_KEY", env::secretKey, "Key for auth relay packets"},
};
// clang-format on

ur::net::udpsocket make_socket(bool ipv6)
{
	auto socket = ur::net::udpsocket::make(ipv6);
	const ur::net::socket_address bindAddr = ipv6 ? ur::net::socket_address::make_ipv6(ur::net::anyIpv6(), 0) : ur::net::socket_address::make_ipv4(ur::net::anyIpv4(), 0);
	if (!socket.bind(bindAddr)) [[unlikely]]
		return ur::net::udpsocket();
	if (!socket.setNonBlocking()) [[unlikely]]
		return ur::net::udpsocket();
	return socket;
}

int main(int argc, char* argv[], char* envp[])
{
	if (ur_init())
		return 1;

	ur::parseArgs(argList, argc, argv);
	ur::parseEnvp(envList, envp);

	if (cl::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	const ur::secret_key key = ur::relay_helpers::makeSecret(env::secretKey);

	auto relayAddr = ur::net::socket_address::from_string(cl::relayAddr);
	relayAddr.setPort(cl::relayPort);

	if (relayAddr.isNull() || !relayAddr.getPort())
	{
		std::println("Invalid address {}", relayAddr);
		return 1;
	}

	for (int32_t i = 0; i < cl::maxProbes; ++i)
	{
		auto socketA = make_socket(relayAddr.isIpv6());
		auto socketB = make_socket(relayAddr.isIpv6());
		if (!socketA.isValid() || !socketB.isValid()) [[unlikely]]
		{
			std::println("Failed to make sockets");
			return 1;
		}

		ur::handshake_header headerPacket{};
		headerPacket.m_magicNumber = ur::handshake_magic_number_hton;
		headerPacket.m_guid = guid::newGuid();
		// custom guid stuff to indicate healthchecks from everything else
		headerPacket.m_guid.m_c = 0x2301FFFF;
		headerPacket.m_guid.m_d = 0xAB986745;

		headerPacket.m_nonce = ur::net::hton64(ur::randRange<uint64_t>(0, UINT64_MAX));
		headerPacket.m_mac = ur::relay_helpers::makeHMAC(key, headerPacket.m_nonce);
		socketA.sendTo(&headerPacket, sizeof(headerPacket), relayAddr);

		headerPacket.m_nonce = ur::net::hton64(ur::randRange<uint64_t>(0, UINT64_MAX));
		headerPacket.m_mac = ur::relay_helpers::makeHMAC(key, headerPacket.m_nonce);
		socketB.sendTo(&headerPacket, sizeof(headerPacket), relayAddr);

		ur::net::socket_address recvAddr{};
		ur::recv_buffer recvBuffer{};
		int32_t recvBytes{};

		const auto waitUntil = std::chrono::steady_clock::now() + cl::waitTime;
		while (waitUntil > std::chrono::steady_clock::now())
		{
			if (socketA.waitForRead(cl::waitTime / 2))
				recvBytes = socketA.recvFrom(recvBuffer.data(), recvBuffer.size(), recvAddr);
			else if (socketB.waitForRead(cl::waitTime / 2))
				recvBytes = socketB.recvFrom(recvBuffer.data(), recvBuffer.size(), recvAddr);

			if (recvBytes > 0)
				break;
		}

		if (recvBytes <= 0)
		{
			std::println("Probe failed {} / {} (no data)", i + 1, cl::maxProbes);
			continue;
		}
		else if (recvBytes != sizeof(ur::handshake_header))
		{
			std::println("Probe failed {} / {} (unexpected size of {} instead {})", i + 1, cl::maxProbes, recvBytes, sizeof(ur::handshake_header));
			continue;
		}
		else if (recvAddr != relayAddr)
		{
			std::println("Huh? Received packet from address {}, when {} expected", recvAddr, relayAddr);
			// it's ok for the most part;
		}

		ur::handshake_header recvHeader{};
		std::memcpy(&recvHeader, recvBuffer.data(), sizeof(recvHeader));

		if (recvHeader.m_magicNumber == headerPacket.m_magicNumber && recvHeader.m_guid == headerPacket.m_guid)
		{
			std::println("Probe succeeded! {} / {}", i + 1, cl::maxProbes);
			return 0; // success!
		}
		else
		{
			std::println("Probe failed {} / {} (unexpected data)", i + 1, cl::maxProbes);
			continue;
		}
	}

	std::println("Healthcheck failed");

	ur_shutdown();

	return 1;
}