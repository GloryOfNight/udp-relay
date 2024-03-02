#pragma once
#include <cstdint>

struct guid
{
	guid() = default;
	guid(int32_t a, int32_t b, int32_t c, int32_t d) 
		: m_a{a}
		, m_b{b}
		, m_c{c}
		, m_d{d}
	{
	}

	int32_t m_a{};
	int32_t m_b{};
	int32_t m_c{};
	int32_t m_d{};

	bool operator<=>(const guid&) const = default;

	std::string toString() const
	{
		char buffer[36]{};
		std::snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%04x%08x", m_a, m_b >> 16, m_b & 0xFFFF, m_c >> 16, m_c & 0xFFFF, m_d);
		return std::string(buffer);
	}
};

struct handshake_header
{
	uint16_t m_type{};
	uint16_t m_length{};
	guid m_id{};
};

// custom hash for guid
namespace std
{
	template <>
	struct hash<guid>
	{
		std::size_t operator()(const guid& g) const
		{
			const int64_t* p = reinterpret_cast<const int64_t*>(&g);
			return std::hash<int64_t>{}(p[0]) ^ std::hash<int64_t>{}(p[1]);
		}
	};
} // namespace std

// custom hash and equal_to for shared_ptr of internetaddr
namespace std
{
	template <>
	struct hash<std::shared_ptr<internetaddr>>
	{
		std::size_t operator()(const std::shared_ptr<internetaddr>& g) const
		{
			if (g.get() == nullptr) [[unlikely]]
				return 0;

			return std::hash<int32_t>{}(g->getIp()) ^ std::hash<uint16_t>{}(g->getPort());
		}
	};

	template <>
	struct equal_to<std::shared_ptr<internetaddr>>
	{
		bool operator()(const std::shared_ptr<internetaddr>& left, const std::shared_ptr<internetaddr>& right) const
		{
			if (left == nullptr || right == nullptr) [[unlikely]]
				return false;

			return *left == *right;
		}
	};
} // namespace std
