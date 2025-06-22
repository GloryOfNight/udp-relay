// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstring>
#include <stdint.h>

namespace ur
{
	// true if current platform little endian
	constexpr bool isLittleEndian() noexcept
	{
		return std::endian::native == std::endian::little;
	}

	// true if current platform big endian
	constexpr bool isBigEndian() noexcept
	{
		return std::endian::native == std::endian::big;
	}

	// swap byte order
	template <std::integral T>
	T byteswap(T value) noexcept;

	// convert host byte order to network
	template <std::integral T>
	T hton(T value) noexcept;

	// convert network byte order to host
	template <std::integral T>
	T ntoh(T value) noexcept;

	// clang-format off
	// Inline wrappers for 16-bit
	inline uint16_t bs16(uint16_t v)	noexcept { return byteswap<uint16_t>(v);}
	inline uint16_t hton16(uint16_t v)	noexcept { return hton<uint16_t>(v);	}
	inline uint16_t ntoh16(uint16_t v)	noexcept { return ntoh<uint16_t>(v);	}

	// Inline wrappers for 32-bit
	inline uint32_t bs32(uint32_t v)	noexcept { return byteswap<uint32_t>(v);}
	inline uint32_t hton32(uint32_t v)	noexcept { return hton<uint32_t>(v);	}
	inline uint32_t ntoh32(uint32_t v)	noexcept { return ntoh<uint32_t>(v);	}

	// Inline wrappers for 64-bit
	inline uint64_t bs64(uint64_t v)	noexcept { return byteswap<uint64_t>(v);}
	inline uint64_t hton64(uint64_t v)	noexcept { return hton<uint64_t>(v);	}
	inline uint64_t ntoh64(uint64_t v)	noexcept { return ntoh<uint64_t>(v);	}
	// clang-format on
} // namespace ur

template <std::integral T>
T ur::byteswap(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
	uint8_t value_representation[sizeof(T)];
	std::memcpy(value_representation, &value, sizeof(T));
	std::ranges::reverse(value_representation);
	return std::bit_cast<T>(value_representation);
}

template <std::integral T>
T ur::hton(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
	if (ur::isLittleEndian())
		return byteswap(value);
	return value;
}

template <std::integral T>
T ur::ntoh(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
	if (ur::isLittleEndian())
		return byteswap(value);
	return value;
}

#define BYTESWAP16(x) ur::byteswap(static_cast<uint16_t>(x))
#define BYTESWAP32(x) ur::byteswap(static_cast<uint32_t>(x))
#define BYTESWAP64(x) ur::byteswap(static_cast<uint64_t>(x))