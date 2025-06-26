// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <chrono>
#include <cstdarg>
#include <format>
#include <iostream>

enum class log_level : uint8_t
{
	NoLogs,
	Fatal,
	Error,
	Warning,
	Info,
	Verbose,
	Debug
};

extern log_level g_logLevel;

namespace udprelaycore
{
	static std::string log_level_to_string(const log_level level)
	{
		switch (level)
		{
		case log_level::Fatal:
			return "Fatal";
		case log_level::Error:
			return "Error";
		case log_level::Warning:
			return "Warning";
		case log_level::Info:
			return "Info";
		case log_level::Verbose:
			return "Verbose";
		case log_level::Debug:
			return "Debug";
		default:
			return "Unknown";
		}
	}

	template <typename... Args>
	void log(const log_level level, const std::string_view category, const std::string_view format, Args... args)
	{
		if (level > g_logLevel)
			return;

		const auto now = std::chrono::utc_clock::now();
		const auto log_level_str = log_level_to_string(level);

		const std::string output = std::vformat("[{0:%F}T{0:%T}] {1}: {2}: ", std::make_format_args(now, category, log_level_str)) + std::vformat(format, std::make_format_args(args...)) + '\n';

		std::ostream& ostream = level == log_level::Error ? std::cerr : std::cout;
		ostream << output;

		if (level == log_level::Fatal) [[unlikely]]
			std::abort();
	}
} // namespace udprelaycore

#define LOG(level, category, format, ...) udprelaycore::log(log_level::level, #category, format, ##__VA_ARGS__);