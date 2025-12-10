// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"
module;

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstring>
#include <stdint.h>

export module ur.net.utils;

export namespace ur::net
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
	constexpr T byteswap(T value) noexcept;

	// convert host byte order to network
	template <std::integral T>
	constexpr T hton(T value) noexcept;

	// convert network byte order to host
	template <std::integral T>
	constexpr T ntoh(T value) noexcept;

	// clang-format off
	// Inline wrappers for 16-bit
	constexpr uint16_t bs16(uint16_t v)	noexcept { return byteswap<uint16_t>(v);	}
	constexpr uint16_t hton16(uint16_t v)	noexcept { return hton<uint16_t>(v);	}
	constexpr uint16_t ntoh16(uint16_t v)	noexcept { return ntoh<uint16_t>(v);	}

	// Inline wrappers for 32-bit
	constexpr uint32_t bs32(uint32_t v)	noexcept { return byteswap<uint32_t>(v);	}
	constexpr uint32_t hton32(uint32_t v)	noexcept { return hton<uint32_t>(v);	}
	constexpr uint32_t ntoh32(uint32_t v)	noexcept { return ntoh<uint32_t>(v);	}

	// Inline wrappers for 64-bit
	constexpr uint64_t bs64(uint64_t v)	noexcept { return byteswap<uint64_t>(v);	}
	constexpr uint64_t hton64(uint64_t v)	noexcept { return hton<uint64_t>(v);	}
	constexpr uint64_t ntoh64(uint64_t v)	noexcept { return ntoh<uint64_t>(v);	}
	// clang-format on
} // namespace ur::net

template <std::integral T>
constexpr T ur::net::byteswap(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
	auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
	std::ranges::reverse(value_representation);
	return std::bit_cast<T>(value_representation);
}

template <std::integral T>
constexpr T ur::net::hton(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
	if (ur::net::isLittleEndian())
		return byteswap(value);
	return value;
}

template <std::integral T>
constexpr T ur::net::ntoh(T value) noexcept
{
	static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
	if (ur::net::isLittleEndian())
		return byteswap(value);
	return value;
}