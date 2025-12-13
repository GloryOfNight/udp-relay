// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/log.hxx"
#include "udp-relay/relay.hxx"
#include "udp-relay/utils.hxx"
#include "udp-relay/val_ref.hxx"

#include <algorithm>
#include <array>
#include <csignal>
#include <memory>
#include <print>
#include <stacktrace>

namespace args
{
	bool printHelp{};										 // when true - prints help and exits
	int32_t logLevel{static_cast<int32_t>(log_level::Info)}; // 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose
	relay_params relayParams{};
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	val_ref{"--help", args::printHelp,																"--help										= print help" },
	val_ref{"--log-level", args::logLevel,															"--log-level <value>						= 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose" },
	val_ref{"--port", args::relayParams.m_primaryPort,												"--port <value>								= main port for accepting requests" },
	val_ref{"--socketRecvBufferSize", args::relayParams.m_socketRecvBufferSize,						"--socketRecvBufferSize <value>             = receive buffer size for internal socket" },
	val_ref{"--socketSendBufferSize", args::relayParams.m_socketSendBufferSize,						"--socketSendBufferSize <value>             = send buffer size for internal socket" },
	val_ref{"--cleanupTimeMs", args::relayParams.m_cleanupTimeMs,									"--cleanupTimeMs <value>					= how often relay should perform clean check" },
	val_ref{"--cleanupInactiveChannelAfterMs", args::relayParams.m_cleanupInactiveChannelAfterMs,	"--cleanupInactiveChannelAfterMs <value>    = inactivity timeout for channel" },
	val_ref{"--expirePacketAfterMs", args::relayParams.m_expirePacketAfterMs,						"--expirePacketAfterMs <value>				= drop packet if packet not relayed within timeout. 0 or negative - do not reattempt relay" },
	val_ref{"--ipv6", args::relayParams.ipv6,														"--ipv6 0|1									= should create and bind to ipv6 socket (dual-stack ipv4/6 mode)" },
};
// clang-format on

static std::string envTest{};
// clang-format off
static constexpr auto envList = std::array
{
	val_ref{"test", envTest, ""},
};
// clang-format on

static relay g_relay{};
static int g_exitCode{};

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

	g_runtimeLogLevel = static_cast<log_level>(args::logLevel);

	if (g_relay.init(args::relayParams))
	{
		g_relay.run();
	}

	return g_exitCode;
}

void handleAbort(int sig)
{
	LOG(Info, Main, "CAUGHT SIGNAL - {0}", sig);

	if (sig == SIGINT || sig == SIGTERM)
		g_relay.stopGracefully();
	else
		g_relay.stop();

	g_exitCode = 128 + sig;
}

void handleCrash(int sig)
{
#if __cpp_lib_stacktrace
	std::println("- - - Caught crash signal {} - - -\nStacktrace:\n{}", sig, std::stacktrace::current());
#else
	std::println("- - - Caught crash signal {} - - -\nStacktrace:\n <compiler doesn't support feature __cpp_lib_stacktrace>", sig);
#endif
	g_exitCode = 128 + sig;
}