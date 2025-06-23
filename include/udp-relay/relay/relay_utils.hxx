// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <cstdint>
#include <string_view>

namespace ur
{
	uint32_t fnv1a_hash(const uint8_t* data, size_t len)
	{
		uint32_t hash = 0x811c9dc5;
		for (size_t i = 0; i < len; ++i)
		{
			hash ^= data[i];
			hash *= 0x01000193;
		}
		return hash;
	}

	uint32_t fnv1a_hash(const std::string_view& str)
	{
		return fnv1a_hash(reinterpret_cast<const uint8_t*>(str.data()), str.size());
	}

} // namespace ur
