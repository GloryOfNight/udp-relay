#pragma once
#include <chrono>
#include <cstdarg>
#include <format>
#include <iostream>

enum class log_level : uint8_t
{
	NoLogs = 0,
	Error = 1,
	Display = 2,
	Verbose = 3
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
		case log_level::Display:
			return "Display";
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

		std::ostream& stream = level == log_level::Error ? std::cerr : std::cout;
		stream << std::vformat("[{0:%F}T{0:%T}] {1}: ", std::make_format_args(std::chrono::utc_clock::now(), log_level_to_string(level))) << std::vformat(format, std::make_format_args(std::forward<Args>(args)...)) << std::endl;
	}
} // namespace udprelaycore

#define LOG(level, format, ...) udprelaycore::log(log_level::level, format, ##__VA_ARGS__);