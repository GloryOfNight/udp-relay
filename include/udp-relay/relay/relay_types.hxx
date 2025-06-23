// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/utils.hxx"

#include <cstdint>
#include <string>

namespace ur
{
	enum class packetType : uint16_t
	{
		First = 0,
		CreateAllocationRequest = 1,
		CreateAllocationResponse = 2,
		ChallengeResponse = 3,
		JoinSessionRequest = 4,
		JoinSessionResponse = 5,
		Last = 6
	};
} // namespace ur

struct guid
{
	guid() = default;
	guid(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
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

	auto operator<=>(const guid&) const = default;

	std::string toString() const
	{
		char buffer[38]{};
		std::snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%04x%08x", m_a, m_b >> 16, m_b & 0xFFFF, m_c >> 16, m_c & 0xFFFF, m_d);
		return std::string(buffer);
	}
};