#include "relay/relay.hxx"

#include "arguments.hxx"
#include "log.hxx"

#include <algorithm>
#include <csignal>

static relay g_relay{};

void handleAbort(int sig);				// handle abort signal from terminal or system
void parseArgs(int argc, char* argv[]); // parse argument list
void parseEnvp(char* envp[]);			// look and parse environment variables we could use

int main(int argc, char* argv[], char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	parseArgs(argc, argv);
	parseEnvp(envp);

	if (!g_relay.init())
	{
		LOG(Error, "Failed to initialize relay. Abrorting.");
		return 1;
	}

	LOG(Display, "Relay initialized. Starting...");

	g_relay.run();

	return 0;
}

void handleAbort(int sig)
{
	LOG(Error, "\nCAUGHT SIGNAL - {0}\n", sig);
	g_relay.stop();
}

void parseArgs(int argc, char* argv[])
{
	val_ref const* prev_arg = nullptr;
	for (int i = 0; i < argc; ++i)
	{
		const std::string_view arg = argv[i];

		// find if argument listed in args
		const auto found_arg = std::find_if(std::begin(args), std::end(args), [&arg](const val_ref& val)
			{ return arg == val.name; });

		// some arguments doesn't need options and some does
		// we sort it out by separation bool and non-bool arguments
		// those arguments that are bool, would be just set true (no options required)
		// for others on next iteration, next value of arg (unless it is a argument) would be their option
		// option would be parsed into one of the possible types of prev_arg
		// for options that are array type, parsing will stop when new argument found or end of list

		if (found_arg != std::end(args))
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
			else if (auto val = prev_arg->to<std::filesystem::path>())
			{
				*val = arg;
				prev_arg = nullptr;
			}
			else if (auto val = prev_arg->to<std::vector<std::filesystem::path>>())
			{
				val->push_back(arg);
			}
			else
			{
				LOG(Error, "Failed parse argument: {0}. Type not supported: {1}", prev_arg->name, prev_arg->type.name());
			}
		}
		else
		{
			if (arg.ends_with("udp-relay") || arg.ends_with("udp-relay.exe"))
				continue;

			LOG(Error, "Unknown argument: {0}", arg.data());
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
			{ return val.name == env_name; });

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
				LOG(Error, "Failed parse environment variable: {0}. Type not supported: {1}", found_env->name, found_env->type.name());
			}
		}
	}
}