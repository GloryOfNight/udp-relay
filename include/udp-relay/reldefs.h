// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#define PLATFORM_WINDOWS _WIN32
#define PLATFORM_LINUX __linux__

#define PLATFORM_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#ifdef _MSC_VER
#define BYTESWAP16(x) _byteswap_ushort(static_cast<uint16_t>(x))
#define BYTESWAP32(x) _byteswap_ulong(static_cast<uint32_t>(x))
#define BYTESWAP64(x) _byteswap_uint64(static_cast<uint64_t>(x))
#elif defined(__clang__)
#define BYTESWAP16(x) __bswap_16(static_cast<uint16_t>(x))
#define BYTESWAP32(x) __bswap_32(static_cast<uint32_t>(x))
#define BYTESWAP64(x) __bswap_64(static_cast<uint64_t>(x))
#endif