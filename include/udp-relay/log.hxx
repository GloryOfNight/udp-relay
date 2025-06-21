// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <chrono>
#include <cstdarg>
#include <format>
#include <iostream>

enum class log_level : uint8_t
{
	NoLogs = 0,
	Error = 1,
	Warning = 2,
	Log = 3,
	Verbose = 4
};

extern log_level g_logLevel;

namespace udprelaycore
{
	static std::string log_level_to_string(const log_level level)
	{
		switch (level)
		{
		case log_level::Error:
			return "Error";
		case log_level::Warning:
			return "Warning";
		case log_level::Log:
			return "Log";
		case log_level::Verbose:
			return "Verbose";
		default:
			return "Unknown";
		}
	}

	template <typename... Args>
	void log(const log_level level, const std::string_view format, Args... args)
	{
		if (level > g_logLevel)
			return;

		std::ostream& ostream = level == log_level::Error ? std::cerr : std::cout;

		const auto now = std::chrono::utc_clock::now();
		const auto log_level_str = log_level_to_string(level);

		ostream << std::vformat("[{0:%F}T{0:%T}] {1}: ", std::make_format_args(now, log_level_str)) << std::vformat(format, std::make_format_args(args...)) << std::endl;
	}
} // namespace udprelaycore

#define LOG(level, format, ...) udprelaycore::log(log_level::level, format, ##__VA_ARGS__);