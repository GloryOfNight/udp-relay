// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/log.hxx"
#include "udp-relay/relay.hxx"
#include "udp-relay/utils.hxx"
#include "udp-relay/val_ref.hxx"

#include <algorithm>
#include <array>
#include <csignal>
#include <memory>

namespace args
{
	bool printHelp{};										 // when true - prints help and exits
	int32_t logLevel{static_cast<int32_t>(log_level::Info)}; // 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose
	int32_t port{};											 // main port for the server
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	val_ref{"--help", args::printHelp,							"--help                         = print help" },
	val_ref{"--log-level", args::logLevel,						"--log-level <value>            = 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose" },
	val_ref{"--port", args::port,								"--port <value>                 = main port for accepting requests" }
};
// clang-format on

static std::string envTest{};
// clang-format off
static constexpr auto envList = std::array
{
	val_ref{"test", envTest, ""},
};
// clang-format on

static std::unique_ptr<relay> g_relay{};
static int g_exitCode{};
log_level g_logLevel{log_level::Info};

static void handleAbort(int sig); // handle abort signal from terminal or system
static void handleCrash(int sig); // handle crash

int relay_main(int argc, char* argv[], char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	std::signal(SIGSEGV, handleCrash);
	std::signal(SIGILL, handleCrash);
	std::signal(SIGFPE, handleCrash);

	ur::parseArgs(argList, argc, argv);
	ur::parseEnvp(envList, envp);

	if (args::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	if (args::port < 0 || args::port > UINT16_MAX)
	{
		LOG(Error, Main, "Invalid primary port number: {0}", args::port);
		return 1;
	}

	g_logLevel = static_cast<log_level>(args::logLevel);

	g_relay = std::make_unique<relay>();

	relay_params params{};
	if (args::port)
		params.m_primaryPort = args::port;

	if (g_relay->init(params))
	{
		g_relay->run();
	}

	return g_exitCode;
}

void handleAbort(int sig)
{
	LOG(Warning, Main, "CAUGHT SIGNAL - {0}", sig);
	if (g_relay)
		g_relay->stop();

	g_exitCode = 128 + sig;
}

void handleCrash(int sig)
{
	LOG(Error, Main, "CRASHED - {0}", sig);
	if (g_relay)
		g_relay->stop();

	g_exitCode = 128 + sig;
}