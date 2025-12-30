#pragma once

#include <array>

namespace ur
{
	template <typename Value, size_t S>
	struct ring_buffer
	{
		using container = std::array<Value, S>;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

		iterator begin() { return m_c.begin(); }
		const_iterator cbegin() const { return m_c.cbegin(); }

		iterator end() { return m_c.end(); }
		const_iterator cend() const { return m_c.cend(); }

		void assign_next(Value v)
		{
			*m_it = v;
			if (++m_it == end())
				m_it = begin();
		}

		iterator find(Value v)
		{
			return std::find(m_c.begin(), m_c.end(), v);
		}

		const_iterator find(Value v) const
		{
			return std::find(m_c.cbegin(), m_c.cend(), v);
		}

	private:
		container m_c{};
		iterator m_it{begin()};
	};
} // namespace ur