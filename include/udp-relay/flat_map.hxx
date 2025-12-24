#pragma once

#include <algorithm>
#include <utility>
#include <vector>

namespace ur
{
	template <typename TKey, typename TValue>
	struct flat_map
	{
		using dataPairType = std::pair<TKey, TValue>;
		using dataType = std::vector<dataPairType>;

		typename dataType::iterator begin() { return m_data.begin(); }
		typename dataType::iterator end() { return m_data.end(); }
		size_t size() const { return m_data.size(); }

		std::pair<typename dataType::iterator, bool> emplace(TKey key, TValue value)
		{
			auto it = find(key);
			if (it != end())
			{
				it->second = std::move(value);
				return {it, false};
			}

			auto insertedIt = m_data.insert(it, {std::move(key), std::move(value)});
			return {insertedIt, true};
		}

		typename dataType::iterator find(const TKey& key)
		{
			auto it = std::lower_bound(begin(), end(), key,
				[](const dataPairType& element, const TKey& val)
				{
					return element.first < val;
				});
			const bool isRightElement = it != end() && it->first == key;
			return isRightElement ? it : end();
		}

		typename dataType::iterator erase(const TKey& key)
		{
			auto it = find(key);
			return it != m_data.end() ? m_data.erase(it) : m_data.end();
		}

		void reserve(typename dataType::size_type amount)
		{
			m_data.reserve(amount);
		}

		dataType m_data{};
	};
} // namespace ur

namespace std
{
	template <typename TKey, typename TValue, typename Predicate>
	size_t erase_if(ur::flat_map<TKey, TValue>& container, Predicate pred)
	{
		const auto oldSize = container.size();
		auto it = std::remove_if(container.begin(), container.end(), pred);
		container.m_data.erase(it, container.end());
		return oldSize - container.size();
	};
} // namespace std