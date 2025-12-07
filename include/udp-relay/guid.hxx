// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "utils.hxx"

import <cstdint>;
import <string>;

struct guid
{
	guid() noexcept = default;
	guid(uint32_t a, uint32_t b, uint32_t c, uint32_t d) noexcept
		: m_a{a}
		, m_b{b}
		, m_c{c}
		, m_d{d}
	{
	}

	static guid newGuid()
	{
		const uint32_t a = ur::randRange<uint32_t>(0, UINT32_MAX);
		const uint32_t b = ur::randRange<uint32_t>(0, UINT32_MAX);
		const uint32_t c = ur::randRange<uint32_t>(0, UINT32_MAX);
		const uint32_t d = ur::randRange<uint32_t>(0, UINT32_MAX);
		return guid(a, b, c, d);
	}

	uint32_t m_a{};
	uint32_t m_b{};
	uint32_t m_c{};
	uint32_t m_d{};

	std::strong_ordering operator<=>(const guid&) const = default;

	std::string toString() const
	{
		char buffer[38]{};
		std::snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%04x%08x", m_a, m_b >> 16, m_b & 0xFFFF, m_c >> 16, m_c & 0xFFFF, m_d);
		return std::string(buffer);
	}

	bool isNull() const noexcept
	{
		return !(m_a || m_b || m_c || m_d);
	}
};

// custom hash for guid
namespace std
{
	template <>
	struct hash<guid>
	{
		std::size_t operator()(const guid& g) const noexcept
		{
			return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(&g), sizeof(g)));
		}
	};

	template <>
	struct less<guid>
	{
		bool operator()(const guid& a, const guid& b) const noexcept
		{
			return std::tie(a.m_a, a.m_b, a.m_c, a.m_d) <
				   std::tie(b.m_a, b.m_b, b.m_c, b.m_d);
		}
	};

} // namespace std