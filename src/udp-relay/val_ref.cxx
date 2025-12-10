// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"
module;

#include <string_view>
#include <typeinfo>

export module ur.val_ref;

export struct val_ref
{
	template <typename T>
	constexpr val_ref(const std::string_view& name, T& value, const std::string_view& noteHelp)
		: m_name{name}
		, m_noteHelp{noteHelp}
		, m_value{&value}
		, m_type{typeid(T)}
	{
	}

	std::string_view m_name;
	std::string_view m_noteHelp;
	void* m_value;
	const std::type_info& m_type;

	template <typename T>
	T* to() const
	{
		return m_type == typeid(T) ? reinterpret_cast<T*>(m_value) : nullptr;
	}
};