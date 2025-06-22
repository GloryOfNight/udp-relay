// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <cstdint>

namespace ur::consts
{
	inline const int32_t sendTimeoutUs = 5000;
	inline const int32_t recvTimeoutUs = 5000;

	inline const int32_t desiredSendBufferSize = 0x10000;
	inline const int32_t desiredRecvBufferSize = 0x10000;
} // namespace ur::consts