// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "udp-relay/networking/network_utils.hxx"
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
		ErrorResponse = 6,
		Last = 7
	};

	enum class errorType : uint16_t
	{
		First = 0,
		Busy = 1,
		WrongPassword = 2,
		Last = 2
	};
} // namespace ur

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

	auto operator<=>(const guid&) const noexcept = default;

	std::string toString() const
	{
		char buffer[38]{};
		std::snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%04x%08x", m_a, m_b >> 16, m_b & 0xFFFF, m_c >> 16, m_c & 0xFFFF, m_d);
		return std::string(buffer);
	}

	static guid hton(guid inGuid) noexcept
	{
		inGuid.m_a = ur::hton32(inGuid.m_a);
		inGuid.m_b = ur::hton32(inGuid.m_b);
		inGuid.m_c = ur::hton32(inGuid.m_c);
		inGuid.m_d = ur::hton32(inGuid.m_d);
		return inGuid;
	}

	static guid ntoh(guid inGuid) noexcept
	{
		inGuid.m_a = ur::ntoh32(inGuid.m_a);
		inGuid.m_b = ur::ntoh32(inGuid.m_b);
		inGuid.m_c = ur::ntoh32(inGuid.m_c);
		inGuid.m_d = ur::ntoh32(inGuid.m_d);
		return inGuid;
	}
};