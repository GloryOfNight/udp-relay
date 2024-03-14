#pragma once
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
	template <typename... Args>
	void log(const log_level level, const std::string_view format, Args... args)
	{
		if (level > g_logLevel)
			return;

		if (level == log_level::Error)
		{
			std::cerr << std::vformat(format, std::make_format_args(std::forward<Args>(args)...)) << std::endl;
		}
		else
		{
			std::cout << std::vformat(format, std::make_format_args(std::forward<Args>(args)...)) << std::endl;
		}
	}
} // namespace cf

#define LOG(level, format, ...) udprelaycore::log(log_level::level, format, ##__VA_ARGS__);