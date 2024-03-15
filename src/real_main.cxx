#include "udp-relay/log.hxx"
#include "udp-relay/relay.hxx"
#include "udp-relay/val_ref.hxx"

#include <algorithm>
#include <array>
#include <csignal>
#include <memory>

namespace args
{
	bool printHelp{};			   // when true - prints help and exits
	int32_t logLevel{};			   // 0 - no logs, 1 - errors only, 2 - warnings only, 3 - log (default), 4 - verbose
	int32_t port{6060};			   // main port for the server
	int32_t warnTickTimeUs{10000}; // log warning when tick exceeded specified time in microseconds
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
static constexpr auto env_vars = std::array
{
	val_ref{"test", envTest, ""},
};
// clang-format on

static std::unique_ptr<relay> g_relay{};

log_level g_logLevel{log_level::Display};

void handleAbort(int sig);				// handle abort signal from terminal or system
void handleCrash(int sig);				// handle crash
void parseArgs(int argc, char* argv[]); // parse argument list
void parseEnvp(char* envp[]);			// look and parse environment variables we could use
void printHelp();						// print help message

int relay_main(int argc, char* argv[], char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	std::signal(SIGSEGV, handleCrash);
	std::signal(SIGILL, handleCrash);
	std::signal(SIGFPE, handleCrash);

	parseArgs(argc, argv);
	parseEnvp(envp);

	if (args::printHelp)
	{
		printHelp();
		return 0;
	}

	if (args::port < 0 || args::port > UINT16_MAX)
	{
		LOG(Error, "Invalid primary port number: {0}", args::port);
		return 1;
	}

	g_logLevel = static_cast<log_level>(args::logLevel);

	LOG(Display, "Starting UDP relay. . .");

	g_relay = std::make_unique<relay>();

	relay_params params{};
	params.m_primaryPort = args::port;
	params.m_warnTickExceedTimeUs = args::warnTickTimeUs;

	g_relay->run(params);

	g_relay.reset();

	return 0;
}

void handleAbort(int sig)
{
	LOG(Error, "CAUGHT SIGNAL - {0}", sig);
	if (g_relay)
		g_relay->stop();
}

void handleCrash(int sig)
{
	LOG(Error, "CRASHED - {0}", sig);
	if (g_relay)
		g_relay->stop();
}

void parseArgs(int argc, char* argv[])
{
	val_ref const* prev_arg = nullptr;
	for (int i = 0; i < argc; ++i)
	{
		const std::string_view arg = argv[i];

		// find if argument listed in args
		const auto found_arg = std::find_if(std::begin(argList), std::end(argList), [&arg](const val_ref& val)
			{ return arg == val.m_name; });

		// some arguments doesn't need options and some does
		// we sort it out by separation bool and non-bool arguments
		// those arguments that are bool, would be just set true (no options required)
		// for others on next iteration, next value of arg (unless it is a argument) would be their option
		// option would be parsed into one of the possible types of prev_arg
		// for options that are array type, parsing will stop when new argument found or end of list

		if (found_arg != std::end(argList))
		{
			prev_arg = &(*found_arg);
			if (auto val = prev_arg->to<bool>())
			{
				*val = true;
				prev_arg = nullptr;
			}
		}
		else if (prev_arg)
		{
			if (auto val = prev_arg->to<int32_t>())
			{
				*val = std::stoi(arg.data());
				prev_arg = nullptr;
			}
			else if (auto val = prev_arg->to<std::string>())
			{
				*val = arg;
				prev_arg = nullptr;
			}
			else
			{
				LOG(Error, "Failed parse argument: {0}. Type not supported: {1}", prev_arg->m_name, prev_arg->m_type.name());
			}
		}
		else
		{
			if (arg.ends_with("udp-relay") || arg.ends_with("udp-relay.exe"))
				continue;

			LOG(Verbose, "Unknown argument: {0}", arg.data());
		}
	}
}

void parseEnvp(char* envp[])
{
	for (int i = 0; envp[i] != NULL; ++i)
	{
		const std::string_view env = envp[i];
		const std::string_view env_name = env.substr(0, env.find_first_of('='));

		// find environment variables that we might need
		const auto found_env = std::find_if(std::begin(env_vars), std::end(env_vars), [&env_name](const val_ref& val)
			{ return val.m_name == env_name; });

		// if found_env variable found
		// separate env variable from it's contents
		// check if found_env are supported type
		// save contents to found_env
		if (found_env != std::end(env_vars))
		{
			const std::string_view env_value = env.substr(env.find_first_of('=') + 1);
			if (auto val = found_env->to<std::string_view>())
			{
				*val = env_value;
			}
			else
			{
				LOG(Error, "Failed parse environment variable: {0}. Type not supported: {1}", found_env->m_name, found_env->m_type.name());
			}
		}
	}
}

void printHelp()
{
	LOG(Display, "Available arguments list:");
	for (auto& arg : argList)
	{
		if (arg.m_noteHelp.size() == 0)
			continue;

		LOG(Display, arg.m_noteHelp);
	}
	LOG(Display, " Apache License Version 2.0 - Copyright (c) 2024 Sergey Dikiy");
}