#pragma once

#include "udp-relay/val_ref.hxx"
#include "udp-relay/log.hxx"

#include <random>
#include <stdint.h>

namespace udprelay::utils
{
	template <typename T>
	T randRange(const T min, const T max)
	{
		static std::random_device rd{};
		static std::mt19937 gen(rd());
		std::uniform_int_distribution<T> dis(min, max);
		return static_cast<T>(dis(gen));
	}

	template <typename T>
	void parseArgs(const T& argList, int argc, char* argv[]); // parse argument list

	template <typename T>
	void printArgsHelp(const T& argList);

	template <typename T>
	void parseEnvp(const T& envList, char* envp[]); // parse environment variables

} // namespace udprelay::utils

template <typename T>
void udprelay::utils::parseArgs(const T& argList, int argc, char* argv[])
{
	static_assert(std::is_same<typename T::value_type, val_ref>::value, "T must be an array of val_ref");

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
			else if (auto val = prev_arg->to<std::vector<int32_t>>())
			{
				val->push_back(std::atoi(arg.data()));
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

template <typename T>
void udprelay::utils::printArgsHelp(const T& argList)
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

template <typename T>
void udprelay::utils::parseEnvp(const T& envList, char* envp[])
{
	static_assert(std::is_same<typename T::value_type, val_ref>::value, "T must be an array of val_ref");

	for (int i = 0; envp[i] != NULL; ++i)
	{
		const std::string_view env = envp[i];
		const std::string_view env_name = env.substr(0, env.find_first_of('='));

		// find environment variables that we might need
		const auto found_env = std::find_if(std::begin(envList), std::end(envList), [&env_name](const val_ref& val)
			{ return val.m_name == env_name; });

		// if found_env variable found
		// separate env variable from it's contents
		// check if found_env are supported type
		// save contents to found_env
		if (found_env != std::end(envList))
		{
			const std::string_view env_value = env.substr(env.find_first_of('=') + 1);

			if (auto val = found_env->template to<std::string_view>())
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