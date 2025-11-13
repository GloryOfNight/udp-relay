// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "networking/internetaddr.hxx"

#include "utils.hxx"

#include <compare>
#include <cstdint>
#include <memory>

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

	bool isValid() const noexcept
	{
		return m_a || m_b || m_c || m_d;
	}
};

const uint32_t handshake_header_magic_number = 0x4B28000;
const uint16_t handshake_header_min_size = 20;
struct handshake_header
{
	uint32_t m_magicNumber{handshake_header_magic_number};
	guid m_guid{};
};
static_assert(sizeof(handshake_header) == handshake_header_min_size);

// custom hash for guid
namespace std
{
	template <>
	struct hash<guid>
	{
		std::size_t operator()(const guid& g) const noexcept
		{
			const uint64_t high = (static_cast<uint64_t>(g.m_a) << 32) | static_cast<uint64_t>(g.m_b);
			const uint64_t low = (static_cast<uint64_t>(g.m_c) << 32) | static_cast<uint64_t>(g.m_d);
			return std::hash<uint64_t>{}(high ^ low);
		}
	};

	template <>
	struct less<guid>
	{
		bool operator()(const guid& left, const guid& right) const noexcept
		{
			if (left.m_a != right.m_a)
				return left.m_a < right.m_a;
			if (left.m_b != right.m_b)
				return left.m_b < right.m_b;
			if (left.m_c != right.m_c)
				return left.m_c < right.m_c;
			return left.m_d < right.m_d;
		}
	};

} // namespace std

// custom hash and equal_to for internetaddr
namespace std
{
	template <>
	struct hash<internetaddr>
	{
		std::size_t operator()(const internetaddr& g) const noexcept
		{
			return std::hash<int32_t>{}(g.getIp() ^ g.getPort());
		}
	};

	template <>
	struct equal_to<internetaddr>
	{
		bool operator()(const internetaddr& left, const internetaddr& right) const noexcept
		{
			return left == right;
		}
	};

	template <>
	struct less<internetaddr>
	{
		bool operator()(const internetaddr& left, const internetaddr& right) const noexcept
		{
			if (left.getIp() != right.getIp())
				return left.getIp() < right.getIp();
			return left.getPort() < right.getPort();
		}
	};
} // namespace std
