// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/log.hxx"
#include "udp-relay/main_helpers.hxx"
#include "udp-relay/relay.hxx"

#include <array>
#include <csignal>
#include <format>
#include <map>
#include <print>
#include <stacktrace>

namespace cl
{
	static bool printHelp{}; // when true - prints help and exits
	static ur::relay_params relayParams{};
} // namespace cl

namespace env
{
	static std::string_view secretKey{};
} // namespace env

// clang-format off
static constexpr auto argList = std::array
{
	ur::cl_var_ref{"--help", cl::printHelp,																"--help										= print help" },
	ur::cl_var_ref{"--log-level", ur::runtime_log_verbosity,												"--log-level 0-4							= set log level no logs - verbose" },
	ur::cl_var_ref{"--port", cl::relayParams.m_primaryPort,												"--port 0-65535								= main port for accepting requests" },
	ur::cl_var_ref{"--socketRecvBufferSize", cl::relayParams.m_socketRecvBufferSize,						"--socketRecvBufferSize <value>             = receive buffer size for internal socket" },
	ur::cl_var_ref{"--socketSendBufferSize", cl::relayParams.m_socketSendBufferSize,						"--socketSendBufferSize <value>             = send buffer size for internal socket" },
	ur::cl_var_ref{"--cleanupTime", cl::relayParams.m_cleanupTime,										"--cleanupTime <value>						= time in ms, how often relay should perform clean check" },
	ur::cl_var_ref{"--cleanupInactiveAfterTime", cl::relayParams.m_cleanupInactiveChannelAfterTime,		"--cleanupInactiveAfterTime <value>			= time in ms, inactivity timeout for channel" },
	ur::cl_var_ref{"--ipv6", cl::relayParams.m_ipv6,														"--ipv6 0|1									= should create and bind to ipv6 socket (dual-stack ipv4/6 mode)" },
	ur::cl_var_ref{"--numForks", cl::relayParams.m_numForks,														"--numForks <1-255>									= (linux-only) amount of forks that server should start" },
};

static constexpr auto envList = std::array
{
	ur::env_var_ref{"UDP_RELAY_SECRET_KEY", env::secretKey, "Key for auth relay packets"},
};
// clang-format on

static void relay_signal_handler(int sig);
static void relay_run();
static void relay_run_forked();

static ur::relay g_relay{};
static int exit_code{};

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

	if (cl::printHelp)
	{
		ur::printArgsHelp(argList);
		return 0;
	}

	if (UR_PLATFORM_LINUX && cl::relayParams.m_numForks)
	{
		relay_run_forked();
	}
	else
	{
		relay_run();
	}

	ur_shutdown();

	return exit_code;
}

void relay_signal_handler(int sig)
{
	LOG(Warning, Main, "Caught signal - {}", sig);

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

	exit_code = 128 + sig;
}

void relay_run()
{
	if (g_relay.init(cl::relayParams, ur::relay_helpers::makeSecret(env::secretKey)))
	{
		g_relay.run();
	}
}

#if UR_PLATFORM_LINUX
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

void relay_run_forked()
{
#if UR_PLATFORM_LINUX
	ur::uses_fork = true;

	LOG(Info, Main, "Starting relay fork instanctes");

	std::map<pid_t, uint8_t> activeForks{};

	for (uint8_t i = 0; i < cl::relayParams.m_numForks; i++)
	{
		const pid_t forkRes = fork();
		if (forkRes == 0) // child process
		{
			ur::fork_id = i + 1;
			cl::relayParams.m_primaryPort += i; // increment port to avoid relay instances try bind same port

			// making sure fork process will terminate if main dies
			prctl(PR_SET_PDEATHSIG, SIGTERM);

			char name[16];
			prctl(PR_GET_NAME, name);

			const std::string newName = std::format("{0}-{1}", name, ur::fork_id.load());
			prctl(PR_SET_NAME, newName.c_str());

			relay_run();
			return;
		}
		else if (forkRes > 0)
		{
			activeForks.emplace(forkRes, i + 1);
			sleep(1);
		}
		else
		{
			LOG(Info, Main, "Failed to start fork instance, errno {} ({})", errno, strerror(errno));
		}
	}

	LOG(Info, Main, "Will wait for forked instanced to exit");

	while (activeForks.size())
	{
		int wstatus{};
		const pid_t waitRes = wait(&wstatus);
		if (waitRes > 0)
		{
			const auto foundFork = activeForks.find(waitRes);
			if (foundFork != activeForks.end())
			{
				LOG(Info, Main, "Fork #{} (PID:{}) exited", foundFork->second, waitRes);
			}
			activeForks.erase(foundFork);
		}
	}
#endif
}