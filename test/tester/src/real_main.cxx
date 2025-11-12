// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay-client/client.hxx"
#include "udp-relay/utils.hxx"
#include "udp-relay/val_ref.hxx"

#include <array>
#include <chrono>
#include <csignal>
#include <functional>
#include <thread>

namespace args
{
	static bool printHelp{};
	static int32_t maxClients{2};
	static std::vector<int32_t> relayAddr{};
	static int32_t relayPort{6060};
	static int32_t shutdownAfter{60};
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	val_ref{"--help", args::printHelp,										"--help                                        = print help" },
	val_ref{"--max-clients", args::maxClients,								"--max-clients                                 = number of clients to create, should be power of 2" },
	val_ref{"--relay-addr", args::relayAddr,								"--relay-addr <value> <value> <value> <value>  = space separated address of relay server, 127 0 0 1 dy default" },
	val_ref{"--relay-port", args::relayPort,								"--relay-port <value>                          = relay server port, 6060 by default" },
	val_ref{"--shutdown-after", args::shutdownAfter,						"--shutdown-after <value>                      = time in seconds after which test will end" },
};
// clang-format on

static std::array<relay_client, 1024> g_clients{};
static bool g_running{};

static void handleAbort(int sig);

int relay_tester_main(int argc, char* argv[], char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	ur::parseArgs(argList, argc, argv);

	if (args::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	if (args::relayAddr.empty())
	{
		args::relayAddr = {127, 0, 0, 1};
	}
	else if (args::relayAddr.size() != 4)
	{
		LOG(Error, RelayTester, "Invalid relay addr size {0}", args::relayAddr.size())
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

	LOG(Info, RelayTester, "Using relay address: {0}.{1}.{2}.{3}:{4}", args::relayAddr[0], args::relayAddr[1], args::relayAddr[2], args::relayAddr[3], args::relayPort);
	LOG(Info, RelayTester, "Starting {0} clients", args::maxClients);

	for (int32_t i = 0; i < args::maxClients; i += 2)
	{
		relay_client_params param;
		param.m_guid = guid::newGuid();
		param.m_sendIntervalMs = 16;

		reinterpret_cast<uint8_t*>(&param.m_server_ip)[0] = args::relayAddr[0];
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[1] = args::relayAddr[1];
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[2] = args::relayAddr[2];
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[3] = args::relayAddr[3];

		param.m_server_port = args::relayPort;

		std::thread(std::bind(&relay_client::run, &g_clients[i], param)).detach();
		std::thread(std::bind(&relay_client::run, &g_clients[i + 1], param)).detach();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	const auto start = std::chrono::steady_clock::now();

	LOG(Info, RelayTester, "Clients started. Probing relay for {0} seconds", args::shutdownAfter);

	g_running = true;
	while (g_running)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		const auto now = std::chrono::steady_clock::now();

		if (args::shutdownAfter > 0 && std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > args::shutdownAfter)
		{
			LOG(Info, RelayTester, "Timesup. Stopping clients.");

			for (size_t i = 0; i < args::maxClients; ++i)
				g_clients[i].stop();

			g_running = false;

			std::this_thread::sleep_for(std::chrono::seconds(2));

			for (size_t i = 0; i < args::maxClients; ++i)
			{
				const auto& client = g_clients[i];
				LOG(Info, RelayTester, "\"{4}\". Median/Average latency: {0} / {1} ms. Sent/Recv packets: {2} / {3}", client.getMedianLatency(), client.getAverageLatency(), client.getPacketsSent(), client.getPacketsRecv(), client.getGuid().toString());
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