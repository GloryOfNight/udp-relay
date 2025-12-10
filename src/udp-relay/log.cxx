// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"
module;

#include <chrono>
#include <format>
#include <print>
#include <string_view>

export module ur.log;

export enum class log_level : uint8_t {
	NoLogs = 0,
	Error = 1,
	Warning = 2,
	Info = 3,
	Verbose = 4
};

export log_level g_runtimeLogLevel{log_level::Info};

#if UR_BUILD_RELEASE
export constexpr log_level g_compileLogLevel{log_level::Info};
#else
export constexpr log_level g_compileLogLevel{log_level::Verbose};
#endif

export namespace ur
{
	const char* log_level_to_string(const log_level level)
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
	void log(const log_level level, const std::string_view category, const std::string_view format, const Args... args)
	{
		if (level > g_runtimeLogLevel)
			return;

		const auto now = std::chrono::utc_clock::now();
		const char* logLevelStr = log_level_to_string(level);
		std::println("{0} {1}", std::vformat("[{0:%F}T{0:%T}] {1}: {2}:", std::make_format_args(now, category, logLevelStr)), std::vformat(format, std::make_format_args(args...)));
	}
} // namespace ur
