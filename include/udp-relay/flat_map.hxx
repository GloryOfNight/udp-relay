#ifndef _GLORYOFNIGHT_FLAT_MAP_
#define _GLORYOFNIGHT_FLAT_MAP_

#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef FLAT_MAP_NAMESPACE
#define FLAT_MAP_NAMESPACE std
#endif

namespace FLAT_MAP_NAMESPACE
{
	template <typename KeyIter, typename ValueIter>
	class flat_map_iterator;

	template <
		class Key,
		class T,
		class Compare = std::less<Key>,
		class KeyContainer = std::vector<Key>,
		class MappedContainer = std::vector<T>>
	class flat_map
	{
	public:
		using key_container_type = KeyContainer;
		using mapped_container_type = MappedContainer;
		using key_type = Key;
		using mapped_type = T;
		using value_type = std::pair<key_type, mapped_type>;
		using key_compare = Compare;
		using reference = std::pair<const key_type&, mapped_type&>;
		using const_reference = std::pair<const key_type&, const mapped_type&>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using iterator = flat_map_iterator<typename key_container_type::iterator, typename mapped_container_type::iterator>;
		using const_iterator = flat_map_iterator<typename key_container_type::const_iterator, typename mapped_container_type::const_iterator>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		struct containers
		{
			key_container_type keys;
			mapped_container_type values;
		};

		iterator begin() noexcept { return iterator(c.keys.begin(), c.values.begin()); }
		const_iterator cbegin() const noexcept { return const_iterator(c.keys.begin(), c.values.begin()); }

		iterator end() noexcept { return iterator(c.keys.end(), c.values.end()); }
		const_iterator cend() const noexcept { return const_iterator(c.keys.end(), c.values.end()); }

		reverse_iterator rbegin() noexcept { return reverse_iterator(begin()); }
		const_reverse_iterator crbegin() noexcept { return const_reverse_iterator(cbegin()); }

		reverse_iterator rend() noexcept { return reverse_iterator(end()); }
		const_reverse_iterator crend() noexcept { return const_reverse_iterator(cend()); }

		iterator find(const key_type& key)
		{
			auto it = lower_bound(key);
			if (it != end() && !compare(key, it->first))
				return it;
			return end();
		}

		const_iterator find(const key_type& key) const
		{
			auto it = lower_bound(key);
			if (it != cend() && !compare(key, it->first))
				return it;
			return cend();
		}

		template <class... Args>
		std::pair<iterator, bool> emplace(Args&&... args)
		{
			value_type v(std::forward<Args>(args)...);
			auto it = lower_bound(v.first);

			if (it != end() && !compare(v.first, it->first))
				return {it, false};

			auto k_pos = c.keys.insert(it.key_it(), std::move(v.first));
			auto v_pos = c.values.insert(it.value_it(), std::move(v.second));
			return {iterator(k_pos, v_pos), true};
		}

		template <class... Args>
		std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args)
		{
			auto it = lower_bound(key);
			if (it != end() && !compare(key, it->first))
				return {it, false};

			auto k_pos = c.keys.insert(it.key_it(), key_type(key));
			auto v_pos = c.values.insert(it.value_it(), mapped_type(std::forward<Args>(args)...));
			return {iterator(k_pos, v_pos), true};
		}

		std::pair<iterator, bool> insert(const value_type& v)
		{
			return emplace(v);
		}

		template <class InputIt>
		void insert(InputIt first, InputIt last)
		{
			for (; first != last; ++first)
			{
				emplace(*first);
			}
		}

		std::pair<iterator, bool> insert_or_assign(const value_type& v)
		{
			auto it = lower_bound(v.first);

			if (it != end() && !compare(v.first, it->first))
			{
				it->second = v.second;
				return {it, false};
			}

			auto k_pos = c.keys.insert(it.key_it(), std::move(v.first));
			auto v_pos = c.values.insert(it.value_it(), std::move(v.second));
			return {iterator(k_pos, v_pos), true};
		}

		iterator erase(iterator pos)
		{
			auto k_it = c.keys.erase(pos.key_it());
			auto v_it = c.values.erase(pos.value_it());
			return iterator(k_it, v_it);
		}

		size_type erase(const key_type& key)
		{
			auto it = find(key);
			if (it != end())
			{
				erase(it);
				return 1;
			}
			return 0;
		}

		void clear() noexcept
		{
			c.keys.clear();
			c.values.clear();
		}

		bool empty() const noexcept { return c.keys.size(); }

		size_type size() const noexcept { return c.keys.size(); }

		size_type max_size() const noexcept { return c.keys.max_size(); }

		iterator lower_bound(const key_type& key)
		{
			auto k_it = std::lower_bound(c.keys.begin(), c.keys.end(), key, compare);
			auto dist = std::distance(c.keys.begin(), k_it);
			auto v_it = std::next(c.values.begin(), dist);
			return iterator(k_it, v_it);
		}

		const_iterator lower_bound(const key_type& key) const
		{
			auto k_it = std::lower_bound(c.keys.begin(), c.keys.end(), key, compare);
			auto dist = std::distance(c.keys.begin(), k_it);
			auto v_it = std::next(c.values.begin(), dist);
			return const_iterator(k_it, v_it);
		}

		void replace(key_container_type&& key_cont, mapped_container_type&& mapped_cont)
		{
			// !!! always assumes that keys sorted and values size == keys size
			c.keys = std::move(key_cont);
			c.values = std::move(mapped_cont);
		}

		containers extract() { return std::move(c); }

		key_compare key_comp() const { return key_compare(); }

		const key_container_type& keys() const noexcept { return c.keys; }

		const mapped_container_type& values() const noexcept { return c.values; }

		T& at(const Key& key)
		{
			const auto it = find(key);
			if (it == end())
				throw std::out_of_range("key doesn't exist");
			return it->second;
		}

		const T& at(const Key& key) const
		{
			const auto it = find(key);
			if (it == end())
				throw std::out_of_range("key doesn't exist");
			return it->second;
		}

		T& operator[](const Key& key) { return try_emplace(key).first->second; }

		T& operator[](Key&& key) { return try_emplace(key).first->second; }

	private:
		containers c;
		key_compare compare;
	};

	template <typename KeyIter, typename ValueIter>
	class flat_map_iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = std::ptrdiff_t;

		using key_type = typename std::iterator_traits<KeyIter>::value_type;
		using mapped_type = typename std::iterator_traits<ValueIter>::value_type;

		struct reference // proxy type, allows (it->first) to work
		{
			const key_type& first;
			mapped_type& second;
			reference* operator->() { return this; }
		};

		flat_map_iterator(KeyIter k, ValueIter v)
			: k_it(k)
			, v_it(v)
		{
		}

		reference operator*() const { return {*k_it, *v_it}; }
		reference operator->() const { return {*k_it, *v_it}; }

		flat_map_iterator& operator++()
		{
			++k_it;
			++v_it;
			return *this;
		}
		flat_map_iterator operator++(int)
		{
			auto tmp = *this;
			++(*this);
			return tmp;
		}
		flat_map_iterator& operator--()
		{
			--k_it;
			--v_it;
			return *this;
		}
		flat_map_iterator& operator+=(difference_type n)
		{
			k_it += n;
			v_it += n;
			return *this;
		}
		flat_map_iterator& operator-=(difference_type n)
		{
			k_it -= n;
			v_it -= n;
			return *this;
		}

		difference_type operator-(const flat_map_iterator& other) const { return k_it - other.k_it; }

		bool operator==(const flat_map_iterator& other) const { return k_it == other.k_it; }
		bool operator!=(const flat_map_iterator& other) const { return !(*this == other); }

		KeyIter key_it() const { return k_it; }
		ValueIter value_it() const { return v_it; }

	private:
		KeyIter k_it;
		ValueIter v_it;
	};
} // namespace FLAT_MAP_NAMESPACE

namespace std
{
	template <class K, class V, class C, class KC, class MC, class Predicate>
	size_t erase_if(FLAT_MAP_NAMESPACE::flat_map<K, V, C, KC, MC>& container, Predicate pred)
	{
		auto c = container.extract();
		auto& keys = c.keys;
		auto& values = c.values;

		auto k_it = keys.begin();
		auto v_it = values.begin();
		auto k_dest = k_it;
		auto v_dest = v_it;

		size_t removed_count = 0;

		for (; k_it != keys.end(); ++k_it, ++v_it)
		{
			if (pred(std::make_pair(std::cref(*k_it), std::ref(*v_it))))
			{
				removed_count++;
			}
			else
			{
				if (k_dest != k_it)
				{
					*k_dest = std::move(*k_it);
					*v_dest = std::move(*v_it);
				}
				++k_dest;
				++v_dest;
			}
		}

		keys.erase(k_dest, keys.end());
		values.erase(v_dest, values.end());

		container.replace(std::move(c.keys), std::move(c.values));

		return removed_count;
	};
} // namespace std

#endif