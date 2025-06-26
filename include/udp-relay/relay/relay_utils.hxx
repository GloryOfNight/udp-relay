// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include "relay_types.hxx"

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

	// return header size in bytes
	constexpr uint16_t getHeaderSize()
	{
		return 24;
	}

	// return min. required packet length for a type
	constexpr uint16_t getMinPacketLengthForType(packetType type)
	{
		switch (type)
		{
		case ur::packetType::CreateAllocationRequest:
			return 20;
		case ur::packetType::CreateAllocationResponse:
			return 0;
		case ur::packetType::ChallengeResponse:
			return 4;
		case ur::packetType::JoinSessionRequest:
			return 0;
		case ur::packetType::JoinSessionResponse:
			return 0;
		case ur::packetType::ErrorResponse:
			return 4;
		default:
			LOG(Fatal, RelayUtils, "getMinPacketLengthForType(): unhandled packet type - {}", static_cast<int>(type));
			return UINT16_MAX;
		}
	}

	// return min. required packet size for a type
	constexpr uint16_t getMinPacketSizeForType(packetType type)
	{
		const uint16_t packetLength = getMinPacketLengthForType(type);
		if (packetLength != UINT16_MAX) [[likely]]
			return getHeaderSize() + packetLength;
		else
			return UINT16_MAX;
	}
} // namespace ur
