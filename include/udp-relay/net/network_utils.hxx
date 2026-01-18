// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <bit>
#include <concepts>
#include <cstdint>

namespace ur::net
{
	// true if current platform little endian
	constexpr bool nativeLittleEndian() noexcept
	{
		return std::endian::native == std::endian::little;
	}

	// true if current platform big endian
	constexpr bool nativeBigEndian() noexcept
	{
		return std::endian::native == std::endian::big;
	}

	// convert host byte order to network
	template <std::unsigned_integral T>
	constexpr T hton(const T v) noexcept
	{
		if constexpr (ur::net::nativeLittleEndian())
			return std::byteswap<T>(v);
		return v;
	}

	// convert network byte order to host
	template <std::unsigned_integral T>
	constexpr T ntoh(const T v) noexcept
	{
		if constexpr (ur::net::nativeLittleEndian())
			return std::byteswap(v);
		return v;
	}

	// clang-format off
	// Inline wrappers for 16-bit
	constexpr uint16_t hton16(const uint16_t v)	noexcept { return hton<uint16_t>(v);	}
	constexpr uint16_t ntoh16(const uint16_t v)	noexcept { return ntoh<uint16_t>(v);	}

	// Inline wrappers for 32-bit
	constexpr uint32_t hton32(const uint32_t v)	noexcept { return hton<uint32_t>(v);	}
	constexpr uint32_t ntoh32(const uint32_t v)	noexcept { return ntoh<uint32_t>(v);	}

	// Inline wrappers for 64-bit
	constexpr uint64_t hton64(const uint64_t v)	noexcept { return hton<uint64_t>(v);	}
	constexpr uint64_t ntoh64(const uint64_t v)	noexcept { return ntoh<uint64_t>(v);	}
	// clang-format on
} // namespace ur::net