// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

import std.compat;

namespace ur
{
	template <std::size_t _alignment, std::size_t _size>
	struct aligned_storage
	{
		aligned_storage()
		{
			static_assert(_size % _alignment == 0, "Invalid size");
			static_assert(_alignment % 2 == 0, "Invalid size");
			m_data = ::operator new(_size, std::align_val_t(_alignment));
		}

		aligned_storage(const aligned_storage&) = delete;
		aligned_storage(aligned_storage&&) = delete;

		~aligned_storage()
		{
			::operator delete(m_data, std::align_val_t(_alignment));
		}

		constexpr std::size_t size() const
		{
			return _size;
		}

		constexpr std::size_t alignment() const
		{
			return _alignment;
		}

		void* data() const
		{
			return m_data;
		}

	private:
		void* m_data{};
	};
} // namespace ur