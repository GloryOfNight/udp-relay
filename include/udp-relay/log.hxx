// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#define LOG(level, category, format, ...)                \
	if constexpr (log_level::level <= g_compileLogLevel) \
		ur::log(log_level::level, #category, format, ##__VA_ARGS__);