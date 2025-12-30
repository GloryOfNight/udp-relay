// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <chrono>
#include <cstdarg>
#include <format>
#include <iostream>
#include <print>

namespace ur
{
	enum class log_level : uint8_t
	{
		NoLogs = 0,
		Error = 1,
		Warning = 2,
		Info = 3,
		Verbose = 4
	};

	extern log_level runtime_log_verbosity;

#if UR_BUILD_RELEASE
	static constexpr log_level compile_log_verbosity{log_level::Info};
#else
	static constexpr log_level compile_log_verbosity{log_level::Verbose};
#endif

	static inline const char* log_level_to_string(const log_level level)
	{
		switch (level)
		{
		case log_level::Error:
			return "Error";
		case log_level::Warning:
			return "Warning";
		case log_level::Info:
			return "Info";
		case log_level::Verbose:
			return "Verbose";
		case log_level::NoLogs:
			return "";
		default:
			return "Unknown";
		}
	}

	template <typename... Args>
	static void log(const log_level level, const std::string_view category, const std::string_view format, const Args... args)
	{
		if (level > runtime_log_verbosity)
			return;

		const auto now = std::chrono::utc_clock::now();
		const char* logLevelStr = log_level_to_string(level);
		std::println(std::cout, "{0} {1}", std::vformat("[{0:%F}T{0:%T}] {1}: {2}:", std::make_format_args(now, category, logLevelStr)), std::vformat(format, std::make_format_args(args...)));
	}

	static inline void log_flush()
	{
		std::cout.flush();
	}
} // namespace ur

#define LOG(level, category, format, ...)                \
	if constexpr (ur::log_level::level <= ur::compile_log_verbosity) \
		ur::log(ur::log_level::level, #category, format, ##__VA_ARGS__);
