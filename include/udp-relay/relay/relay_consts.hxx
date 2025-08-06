// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <cstdint>

namespace ur::consts
{
	inline const int32_t sendTimeoutUs = 5000; // how much time in microseconds should pass before Send returns with timeout error
	inline const int32_t recvTimeoutUs = 5000; // how much time in microseconds should pass before Recv returns with timeout error

	inline const int32_t desiredSendBufferSize = 0x10000; // send buffer size for socket
	inline const int32_t desiredRecvBufferSize = 0x10000; // receive buffer size for socket

	inline const int32_t magicCookie = 0x37e7c7e3; // cookie to identify relay packets from everything else

	inline const int32_t maxClients = 64; // maximum amount of clients supported
} // namespace ur::consts