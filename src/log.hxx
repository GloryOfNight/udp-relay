#pragma once
#include <cstdarg>
#include <format>
#include <iostream>

enum class log_level : uint8_t
{
	NoLogs,
	Verbose,
	Display,
	Error
};

extern log_level gLogLevel;

namespace cf
{
	template <typename... Args>
	void log(const log_level level, const std::string_view format, Args... args)
	{
		if (gLogLevel <= log_level::NoLogs || level < gLogLevel)
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

#define LOG(level, format, ...) cf::log(log_level::level, format, ##__VA_ARGS__);