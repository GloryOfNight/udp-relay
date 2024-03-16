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
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	val_ref{"--help", args::printHelp,										"--help                         = print help" },
	val_ref{"--max-clients", args::maxClients,								"" },
	val_ref{"--relay-addr", args::relayAddr,								"" },
	val_ref{"--relay-port", args::relayPort,								"" },

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

	udprelay::utils::parseArgs(argList, argc, argv);

	if (args::printHelp)
	{
		udprelay::utils::printArgsHelp(argList);
		return 0;
	}

	if (args::relayAddr.size() != 4)
	{
		LOG(Error, "Invalid relay addr size {0}", args::relayAddr.size())
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

	for (int32_t i = 0; i < args::maxClients; i += 2)
	{
		relay_client_params param;
		param.m_guid = guid::newGuid();
		param.m_sendIntervalMs = 500;
		
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[0] = 172;
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[1] = 0;
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[2] = 0;
		reinterpret_cast<uint8_t*>(&param.m_server_ip)[3] = 1;

		param.m_server_port = args::relayPort;
		param.m_sleepMs = 100;

		std::thread(std::bind(&relay_client::run, &g_clients[i], param)).detach();
		std::thread(std::bind(&relay_client::run, &g_clients[i + 1], param)).detach();
	}

	g_running = true;
	while (g_running)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}

void handleAbort(int sig)
{
	LOG(Error, "CAUGHT SIGNAL - {0}", sig);
	for (auto& client : g_clients)
	{
		client.stop();
	}
	g_running = false;
}