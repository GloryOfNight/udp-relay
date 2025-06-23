// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/networking/network_utils.hxx"

#include "udp-relay/platform.h"

#if PLATFORM_WINDOWS
#include <winsock2.h>
#elif PLATFORM_LINUX
#include <cstring>
#include <errno.h>
#endif

int32_t ur::getLastErrno() noexcept
{
#if PLATFORM_WINDOWS
	return WSAGetLastError();
#elif PLATFORM_LINUX
	return errno;
#else
	static_assert(false, "unimplemented");
	return -1;
#endif
}

std::string ur::errnoToString(int32_t value)
{
#if PLATFORM_WINDOWS
	DWORD errCode = value;
	char buf[256];
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errCode,
		0,
		buf,
		sizeof(buf),
		nullptr);
	return std::string(buf);
#else
	return std::strerror(value);
#endif
}