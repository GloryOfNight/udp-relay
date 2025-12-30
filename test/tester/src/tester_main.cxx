// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/utils.hxx"

#include "client.hxx"

#include <array>
#include <chrono>
#include <csignal>
#include <functional>
#include <thread>

using namespace std::chrono_literals;

namespace args
{
	static bool printHelp{};
	static int32_t maxClients{2};
	static std::string relayAddr{};
	static uint16_t relayPort{6060};
	static int32_t shutdownAfter{15};
	static std::chrono::milliseconds sendIntervalMs{24ms};
	static bool useIpv6{false};
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	cl_arg_ref{"--help", args::printHelp,									"--help											= print help" },
	cl_arg_ref{"--max-clients", args::maxClients,							"--max-clients									= number of clients to create, should be power of 2" },
	cl_arg_ref{"--relay-addr", args::relayAddr,								"--relay-addr <value> <value> <value> <value>	= space separated address of relay server, 127 0 0 1 dy default" },
	cl_arg_ref{"--relay-port", args::relayPort,								"--relay-port <value>							= relay server port, 6060 by default" },
	cl_arg_ref{"--shutdown-after", args::shutdownAfter,						"--shutdown-after <value>						= time in seconds after which test will end" },
	cl_arg_ref{"--send-interval-ms", args::sendIntervalMs,					"--send-interval-ms <value>						= how often client should send" },
	cl_arg_ref{"--ipv6", args::useIpv6,										"--ipv6 0|1										= use ipv6" },
};
// clang-format on

// clang-format off
static std::string secretKey{};
static constexpr auto envList = std::array
{
	env_var_ref{"UDP_RELAY_SECRET_KEY", secretKey, "Key for auth relay packets"},
};
// clang-format on

static std::array<relay_client, 1024> g_clients{};
static bool g_running{};

static void handleAbort(int sig);

int main(int argc, char* argv[], [[maybe_unused]] char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	ur::parseArgs(argList, argc, argv);
	ur::parseEnvp(envList, envp);

	if (args::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	if (args::relayAddr.empty())
	{
		args::relayAddr = args::useIpv6 ? "::1" : "127.0.0.1";
	}

	socket_address relayAddr = socket_address::from_string(args::relayAddr);
	relayAddr.setPort(args::relayPort);
	if (relayAddr.isNull() && relayAddr.getPort() != 0)
	{
		LOG(Error, RelayTester, "Invalid relay addr {}, port {}", args::relayAddr, args::relayPort)
		return 1;
	}

	if (relayAddr.isIpv4() && args::useIpv6)
	{
		LOG(Error, RelayTester, "Conflicting arguments. Cannot use ipv4 addr while ipv6 enabled");
		return 1;
	}

	if (args::maxClients % 2 != 0)
	{
		++args::maxClients;
	}
	if (args::maxClients > 1024)
	{
		args::maxClients = 1024;
	}

	LOG(Info, RelayTester, "Using relay address: {}", relayAddr);
	LOG(Info, RelayTester, "Starting {0} clients", args::maxClients);

	for (int32_t i = 0; i < args::maxClients; i += 2)
	{
		relay_client_params params{};
		params.m_guid = guid::newGuid();
		params.m_sendIntervalMs = args::sendIntervalMs;
		params.m_relayAddr = relayAddr;

		auto clientA = &g_clients[i];
		auto clientB = &g_clients[i + 1];

		clientA->init(params, ur::relay_helpers::makeSecret(secretKey));
		clientB->init(params, ur::relay_helpers::makeSecret(secretKey));

		std::thread(std::bind(&relay_client::run, clientA)).detach();
		std::thread(std::bind(&relay_client::run, clientB)).detach();
	}

	const auto start = std::chrono::steady_clock::now();

	LOG(Info, RelayTester, "Clients started. Probing relay for {0} seconds", args::shutdownAfter);

	g_running = true;
	while (g_running)
	{
		std::this_thread::sleep_for(1s);

		const auto now = std::chrono::steady_clock::now();

		if (args::shutdownAfter > 0 && std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > args::shutdownAfter)
		{
			LOG(Info, RelayTester, "Times up. Stopping clients.");

			for (size_t i = 0; i < args::maxClients; ++i)
				g_clients[i].stopSending();

			std::this_thread::sleep_for(2s);

			for (size_t i = 0; i < args::maxClients; ++i)
				g_clients[i].stop();

			g_running = false;

			std::this_thread::sleep_for(2s);

			for (size_t i = 0; i < args::maxClients; ++i)
			{
				const auto& client = g_clients[i];

				const double successRate = client.getPacketsSent() != 0 ? static_cast<double>(client.getPacketsRecv()) / static_cast<double>(client.getPacketsSent()) : 0.;

				LOG(Info, RelayTester, "\"{4}\". Median/Average latency: {0} / {1} ms. Sent/Recv packets: {2} / {3} ({5:.1f} %)", client.getMedianLatency(), client.getAverageLatency(), client.getPacketsSent(), client.getPacketsRecv(), client.getGuid().toString(), successRate * 100.);
			}
		}
	}

	return 0;
}

void handleAbort(int sig)
{
	LOG(Error, RelayTester, "CAUGHT SIGNAL - {0}", sig);
	for (auto& client : g_clients)
	{
		client.stop();
	}
	g_running = false;
}