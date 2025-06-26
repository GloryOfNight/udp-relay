// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "networking/internetaddr.hxx"
#include "relay/relay_types.hxx"

#include "utils.hxx"

#include <cstdint>
#include <memory>

struct handshake_header
{
	uint16_t m_type{};
	uint16_t m_length{};
	guid m_guid{};
	int64_t m_time{};
};

// custom hash for guid
namespace std
{
	template <>
	struct hash<guid>
	{
		std::size_t operator()(const guid& g) const
		{
			const int64_t* p = reinterpret_cast<const int64_t*>(&g);
			return std::hash<int64_t>{}(p[0]) ^ std::hash<int64_t>{}(p[1]);
		}
	};

	template <>
	struct less<guid>
	{
		bool operator()(const guid& left, const guid& right) const
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
		std::size_t operator()(const internetaddr& g) const
		{
			return std::hash<int32_t>{}(g.getIp()) ^ std::hash<uint16_t>{}(g.getPort());
		}
	};

	template <>
	struct equal_to<internetaddr>
	{
		bool operator()(const internetaddr& left, const internetaddr& right) const
		{
			return left == right;
		}
	};

	template <>
	struct less<internetaddr>
	{
		bool operator()(const internetaddr& left, const internetaddr& right) const
		{
			if (left.getIp() != right.getIp())
				return left.getIp() < right.getIp();
			return left.getPort() < right.getPort();
		}
	};
} // namespace std
