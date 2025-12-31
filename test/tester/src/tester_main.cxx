// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/log.hxx"
#include "udp-relay/main_helpers.hxx"

#include "client.hxx"

#include <array>
#include <chrono>
#include <csignal>
#include <functional>
#include <stacktrace>
#include <thread>

using namespace std::chrono_literals;

namespace cl
{
	static bool printHelp{};
	static int32_t maxClients{2};
	static std::string relayAddr{};
	static uint16_t relayPort{6060};
	static int32_t shutdownAfter{15};
	static std::chrono::milliseconds sendIntervalMs{24ms};
	static bool useIpv6{false};
} // namespace cl

namespace env
{
	static std::string_view secretKey{};
} // namespace env

// clang-format off
static constexpr auto argList = std::array
{
	ur::cl_var_ref{"--help", cl::printHelp,									"--help											= print help" },
	ur::cl_var_ref{"--max-clients", cl::maxClients,							"--max-clients									= number of clients to create, should be power of 2" },
	ur::cl_var_ref{"--relay-addr", cl::relayAddr,								"--relay-addr <value> <value> <value> <value>	= space separated address of relay server, 127 0 0 1 dy default" },
	ur::cl_var_ref{"--relay-port", cl::relayPort,								"--relay-port <value>							= relay server port, 6060 by default" },
	ur::cl_var_ref{"--shutdown-after", cl::shutdownAfter,						"--shutdown-after <value>						= time in seconds after which test will end" },
	ur::cl_var_ref{"--send-interval-ms", cl::sendIntervalMs,					"--send-interval-ms <value>						= how often client should send" },
	ur::cl_var_ref{"--ipv6", cl::useIpv6,										"--ipv6 0|1										= use ipv6" },
};

static constexpr auto envList = std::array
{
	ur::env_var_ref{"UDP_RELAY_SECRET_KEY", env::secretKey, "Key for auth relay packets"},
};
// clang-format on

static std::array<relay_client, 1024> g_clients{};
static bool g_running{};
static int exit_code{};

static void tester_signal_handler(int sig);

int main(int argc, char* argv[], [[maybe_unused]] char* envp[])
{
	ur_init();

	std::signal(SIGINT, tester_signal_handler);
	std::signal(SIGTERM, tester_signal_handler);
	std::signal(SIGABRT, tester_signal_handler);
	std::signal(SIGSEGV, tester_signal_handler);
	std::signal(SIGILL, tester_signal_handler);
	std::signal(SIGFPE, tester_signal_handler);

	ur::parseArgs(argList, argc, argv);
	ur::parseEnvp(envList, envp);

	if (cl::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	if (cl::relayAddr.empty())
	{
		cl::relayAddr = cl::useIpv6 ? "::1" : "127.0.0.1";
	}

	auto relayAddr = ur::net::socket_address::from_string(cl::relayAddr);
	relayAddr.setPort(cl::relayPort);
	if (relayAddr.isNull() && relayAddr.getPort() != 0)
	{
		LOG(Error, RelayTester, "Invalid relay addr {}, port {}", cl::relayAddr, cl::relayPort)
		return 1;
	}

	if (relayAddr.isIpv4() && cl::useIpv6)
	{
		LOG(Error, RelayTester, "Conflicting arguments. Cannot use ipv4 addr while ipv6 enabled");
		return 1;
	}

	if (cl::maxClients % 2 != 0)
	{
		++cl::maxClients;
	}
	if (cl::maxClients > 1024)
	{
		cl::maxClients = 1024;
	}

	LOG(Info, RelayTester, "Using relay address: {}", relayAddr);
	LOG(Info, RelayTester, "Starting {0} clients", cl::maxClients);

	for (int32_t i = 0; i < cl::maxClients; i += 2)
	{
		relay_client_params params{};
		params.m_guid = guid::newGuid();
		params.m_sendIntervalMs = cl::sendIntervalMs;
		params.m_relayAddr = relayAddr;

		auto clientA = &g_clients[i];
		auto clientB = &g_clients[i + 1];

		clientA->init(params, ur::relay_helpers::makeSecret(env::secretKey));
		clientB->init(params, ur::relay_helpers::makeSecret(env::secretKey));

		std::thread(std::bind(&relay_client::run, clientA)).detach();
		std::thread(std::bind(&relay_client::run, clientB)).detach();
	}

	const auto start = std::chrono::steady_clock::now();

	LOG(Info, RelayTester, "Clients started. Probing relay for {0} seconds", cl::shutdownAfter);

	g_running = true;
	while (g_running)
	{
		std::this_thread::sleep_for(1s);

		const auto now = std::chrono::steady_clock::now();

		if (cl::shutdownAfter > 0 && std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > cl::shutdownAfter)
		{
			LOG(Info, RelayTester, "Times up. Stopping clients.");

			for (int32_t i = 0; i < cl::maxClients; ++i)
				g_clients[i].stopSending();

			std::this_thread::sleep_for(2s);

			for (int32_t i = 0; i < cl::maxClients; ++i)
				g_clients[i].stop();

			g_running = false;

			std::this_thread::sleep_for(2s);

			for (int32_t i = 0; i < cl::maxClients; ++i)
			{
				const auto& client = g_clients[i];

				const double successRate = client.getPacketsSent() != 0 ? static_cast<double>(client.getPacketsRecv()) / static_cast<double>(client.getPacketsSent()) : 0.;

				LOG(Info, RelayTester, "\"{4}\". Median/Average latency: {0} / {1} ms. Sent/Recv packets: {2} / {3} ({5:.1f} %)", client.getMedianLatency(), client.getAverageLatency(), client.getPacketsSent(), client.getPacketsRecv(), client.getGuid().toString(), successRate * 100.);
			}
		}
	}

	ur_shutdown();

	return exit_code;
}

void tester_signal_handler(int sig)
{
	std::println("- - - Caught signal {} - - -", sig);

	switch (sig)
	{
	case SIGINT:
	case SIGTERM:
		for (auto& client : g_clients)
			client.stop();
		break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGABRT:
		std::println("Stacktrace:\n{}", sig, std::stacktrace::current());
		break;
	default:
		break;
	}

	exit_code = 128 + sig;
}