#pragma once

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace ur
{
	template <
		typename Key,
		typename Value,
		typename Compare = std::less<Key>>
	class flat_map
	{
	public:
		using key_type = Key;
		using mapped_type = Value;
		using key_compare = Compare;

		using value_type = std::pair<key_type, mapped_type>;
		using container_type = std::vector<value_type>;

		using iterator = container_type::iterator;
		using const_iterator = container_type::const_iterator;
		using size_type = container_type::size_type;

		iterator begin() { return m_data.begin(); }
		iterator end() { return m_data.end(); }

		size_t size() const { return m_data.size(); }

		void reserve(size_type newCap) { m_data.reserve(newCap); }
		size_type capacity() const { return m_data.capacity(); }

		std::pair<iterator, bool> emplace(key_type key, mapped_type value)
		{
			auto it = lower_bound_helper(key);
			if (it != m_data.end() && !(key_compare{}(key, it->first)))
			{
				it->second = std::move(value);
				return {it, false};
			}
			auto insertedIt = m_data.insert(it, {std::move(key), std::move(value)});
			return {insertedIt, true};
		}

		iterator find(const key_type& key)
		{
			auto it = lower_bound_helper(key);
			if (it != m_data.end() && !(key_compare{}(key, it->first)))
			{
				return it;
			}
			return m_data.end();
		}

		iterator erase(const key_type& key)
		{
			auto it = find(key);
			return it != m_data.end() ? m_data.erase(it) : m_data.end();
		}

		iterator erase(iterator itBegin, iterator itEnd)
		{
			return m_data.erase(itBegin, itEnd);
		}

	private:
		iterator lower_bound_helper(const key_type& key)
		{
			return std::lower_bound(m_data.begin(), m_data.end(), key,
				[](const value_type& element, const key_type& val)
				{
					return key_compare{}(element.first, val);
				});
		}

		container_type m_data{};
	};
} // namespace ur

namespace std
{
	template <typename Key, typename Value, typename Predicate>
	size_t erase_if(ur::flat_map<Key, Value>& container, Predicate pred)
	{
		const auto oldSize = container.size();
		auto it = std::remove_if(container.begin(), container.end(), pred);
		container.erase(it, container.end());
		return oldSize - container.size();
	};
} // namespace std