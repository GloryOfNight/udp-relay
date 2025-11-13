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
	Info = 3,
	Verbose = 4
};

extern log_level g_runtimeLogLevel;

#if UR_BUILD_RELEASE
static constexpr log_level g_compileLogLevel{log_level::Info};
#else
static constexpr log_level g_compileLogLevel{log_level::Verbose};
#endif

namespace ur::core::log
{
	static const char* log_level_to_string(const log_level level)
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
		default:
			return "Unknown";
		}
	}

	template <typename... Args>
	void log(const log_level level, const std::string_view category, const std::string_view format, Args... args)
	{
		if (level > g_runtimeLogLevel)
			return;

		const auto now = std::chrono::utc_clock::now();
		const char* logLevelStr = log_level_to_string(level);
		const std::string logBase = std::vformat("[{0:%F}T{0:%T}] {1}: {2}:", std::make_format_args(now, category, logLevelStr));
		const std::string logMessage = std::vformat(format, std::make_format_args(args...));
		const std::string logFinal = std::vformat("{0} {1}\n", std::make_format_args(logBase, logMessage));

		std::ostream& ostream = level == log_level::Error ? std::cerr : std::cout;
		ostream << logFinal;
	}

	static inline void flush()
	{
		std::cout.flush();
	}
} // namespace ur::core::log

#define LOG(level, category, format, ...)                \
	if constexpr (log_level::level <= g_compileLogLevel) \
		ur::core::log::log(log_level::level, #category, format, ##__VA_ARGS__);

#define LOG_FLUSH() \
	ur::core::log::flush();
