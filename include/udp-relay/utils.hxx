// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <random>

namespace ur
{
	// generate random value in range
	template <typename T>
	T randRange(const T min, const T max);
} // namespace ur

template <typename T>
T ur::randRange(const T min, const T max)
{
	static std::random_device rd{};
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<T> dis(min, max);
	return static_cast<T>(dis(gen));
}