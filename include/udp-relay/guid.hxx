// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "net/network_utils.hxx"

#include "utils.hxx"

#include <cstdint>
#include <format>
#include <string>

struct guid
{
	guid() noexcept = default;
	constexpr guid(uint32_t a, uint32_t b, uint32_t c, uint32_t d) noexcept
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
		return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:04x}{:08x}", m_a, m_b >> 16, m_b & 0xFFFF, m_c >> 16, m_c & 0xFFFF, m_d);
	}

	bool isNull() const noexcept
	{
		return !(m_a || m_b || m_c || m_d);
	}
};

namespace ur::net
{
	constexpr guid hton(const guid& v)
	{
		return guid(hton32(v.m_a), hton32(v.m_b), hton32(v.m_c), hton32(v.m_d));
	}

	constexpr guid ntoh(const guid& v)
	{
		return guid(ntoh32(v.m_a), ntoh32(v.m_b), ntoh32(v.m_c), ntoh32(v.m_d));
	}
} // namespace ur::net

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

	template <>
	struct formatter<guid>
	{
		constexpr auto parse(format_parse_context& ctx)
		{
			return ctx.begin();
		}

		template <typename FormatContext>
		auto format(const guid& value, FormatContext& ctx) const
		{
			return std::format_to(ctx.out(), "{}", value.toString());
		}
	};
} // namespace std
