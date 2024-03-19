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
	bool printHelp{};											// when true - prints help and exits
	int32_t logLevel{static_cast<int32_t>(log_level::Display)}; // 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose
	int32_t port{6060};											// main port for the server
	int32_t warnTickTimeUs{5000};								// log warning when tick exceeded specified time in microseconds
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	val_ref{"--help", args::printHelp,							"--help                         = print help" },
	val_ref{"--log-level", args::logLevel,						"--log-level <value>            = 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose" },
	val_ref{"--port", args::port,								"--port <value>                 = main port for accepting requests" },
	val_ref{"--warn-tick-time", args::warnTickTimeUs,			"--warn-tick-time <value>       = log warning when tick exceeded specified time in microseconds" },
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
log_level g_logLevel{log_level::Display};

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

	udprelay::utils::parseArgs(argList, argc, argv);
	udprelay::utils::parseEnvp(envList, envp);

	if (args::printHelp)
	{
		udprelay::utils::printArgsHelp(argList);
		return 0;
	}

	if (args::port < 0 || args::port > UINT16_MAX)
	{
		LOG(Error, "Invalid primary port number: {0}", args::port);
		return 1;
	}

	g_logLevel = static_cast<log_level>(args::logLevel);

	LOG(Display, "Starting UDP relay. . .");
	LOG(Display, "Buffered send version");

	g_relay = std::make_unique<relay>();

	relay_params params{};
	params.m_primaryPort = args::port;
	params.m_warnTickExceedTimeUs = args::warnTickTimeUs;

	g_relay->run(params);

	g_relay.reset();

	return g_exitCode;
}

void handleAbort(int sig)
{
	LOG(Error, "CAUGHT SIGNAL - {0}", sig);
	if (g_relay)
		g_relay->stop();

	g_exitCode = 128 + sig;
}

void handleCrash(int sig)
{
	LOG(Error, "CRASHED - {0}", sig);
	if (g_relay)
		g_relay->stop();

	g_exitCode = 128 + sig;
}