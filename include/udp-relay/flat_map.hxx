#pragma once

#include <algorithm>
#include <functional>
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
			auto it = lower_bound_helper(key);
			if (it != m_data.end() && !(std::less<TKey>{}(key, it->first)))
			{
				it->second = std::move(value);
				return {it, false};
			}
			auto insertedIt = m_data.insert(it, {std::move(key), std::move(value)});
			return {insertedIt, true};
		}

		typename dataType::iterator find(const TKey& key)
		{
			auto it = lower_bound_helper(key);
			if (it != m_data.end() && !(std::less<TKey>{}(key, it->first)))
			{
				return it;
			}
			return m_data.end();
		}

		typename dataType::iterator erase(const TKey& key)
		{
			auto it = find(key);
			return it != m_data.end() ? m_data.erase(it) : m_data.end();
		}

		typename dataType::iterator erase(dataType::iterator itBegin, dataType::iterator itEnd)
		{
			return m_data.erase(itBegin, itEnd);
		}

		void reserve(typename dataType::size_type amount)
		{
			m_data.reserve(amount);
		}

	private:
		typename dataType::iterator lower_bound_helper(const TKey& key)
		{
			return std::lower_bound(m_data.begin(), m_data.end(), key,
				[](const dataPairType& element, const TKey& val)
				{
					return std::less<TKey>{}(element.first, val);
				});
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
		container.erase(it, container.end());
		return oldSize - container.size();
	};
} // namespace std