// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/log.hxx"
#include "udp-relay/relay.hxx"
#include "udp-relay/utils.hxx"

#include <algorithm>
#include <array>
#include <csignal>
#include <memory>
#include <print>
#include <stacktrace>

namespace args
{
	static bool printHelp{}; // when true - prints help and exits
	static ur::relay_params relayParams{};
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	cl_arg_ref{"--help", args::printHelp,																"--help										= print help" },
	cl_arg_ref{"--log-level", ur::runtime_log_verbosity,												"--log-level 0-4							= set log level no logs - verbose" },
	cl_arg_ref{"--port", args::relayParams.m_primaryPort,												"--port 0-65535								= main port for accepting requests" },
	cl_arg_ref{"--socketRecvBufferSize", args::relayParams.m_socketRecvBufferSize,						"--socketRecvBufferSize <value>             = receive buffer size for internal socket" },
	cl_arg_ref{"--socketSendBufferSize", args::relayParams.m_socketSendBufferSize,						"--socketSendBufferSize <value>             = send buffer size for internal socket" },
	cl_arg_ref{"--cleanupTime", args::relayParams.m_cleanupTime,										"--cleanupTime <value>						= time in ms, how often relay should perform clean check" },
	cl_arg_ref{"--cleanupInactiveAfterTime", args::relayParams.m_cleanupInactiveChannelAfterTime,		"--cleanupInactiveAfterTime <value>			= time in ms, inactivity timeout for channel" },
	cl_arg_ref{"--ipv6", args::relayParams.ipv6,														"--ipv6 0|1									= should create and bind to ipv6 socket (dual-stack ipv4/6 mode)" },
};
// clang-format on

// clang-format off
static std::string secretKey{};
static constexpr auto envList = std::array
{
	env_var_ref{"UDP_RELAY_SECRET_KEY", secretKey, "Key for auth relay packets"},
};
// clang-format on

static void relay_signal_handler(int sig);

static ur::relay g_relay{};
static int g_exit_code{};

int main(int argc, char* argv[], char* envp[])
{
	if (ur_init())
		return 1;

	std::signal(SIGINT, relay_signal_handler);
	std::signal(SIGTERM, relay_signal_handler);
	std::signal(SIGABRT, relay_signal_handler);
	std::signal(SIGSEGV, relay_signal_handler);
	std::signal(SIGILL, relay_signal_handler);
	std::signal(SIGFPE, relay_signal_handler);

	ur::parseArgs(argList, argc, argv);
	ur::parseEnvp(envList, envp);

	if (args::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	if (g_relay.init(args::relayParams, ur::relay_helpers::makeSecret(secretKey)))
	{
		g_relay.run();
	}

	ur_shutdown();

	return g_exit_code;
}

void relay_signal_handler(int sig)
{
	std::println("- - - Caught signal {} - - -", sig);

	switch (sig)
	{
	case SIGINT:
		g_relay.stop();
		break;
	case SIGTERM:
		g_relay.stopGracefully();
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

	g_exit_code = 128 + sig;
}
