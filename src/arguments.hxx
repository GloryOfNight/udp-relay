#pragma once
#include <array>
#include <string>
#include <vector>

struct val_ref
{
	template <typename T>
	constexpr val_ref(const std::string_view inName, T& inValue, const std::string_view inNoteHelp)
		: name{inName}
		, note_help{inNoteHelp}
		, value{&inValue}
		, type{typeid(T)}
	{
	}

	std::string_view name;
	std::string_view note_help;
	void* value;
	const std::type_info& type;

	template <typename T>
	T* to() const
	{
		return type == typeid(T) ? reinterpret_cast<T*>(value) : nullptr;
	}
};

namespace args
{
	extern bool printHelp;	// when true - prints help and exits
	extern bool logDisable; // when true - disable all logs
	extern bool logVerbose; // when true - log verbose messages
	extern int32_t port;	// main port for the server
} // namespace args

// clang-format off
static constexpr auto argList = std::array
{
	val_ref{"--help", args::printHelp,			"--help                         = print help" },
	val_ref{"--no-logs", args::logDisable,		"--no-logs                      = disable logs (might improve performance slightly)" },
	val_ref{"--verbose", args::logVerbose,		"--verbose                      = enable verbose logs" },
	val_ref{"--port", args::port,				"--port <value>                 = main port for accepting requests" },
};
// clang-format on

static std::string envTest{};
// clang-format off
static constexpr auto env_vars = std::array
{
	val_ref{"test", envTest, ""},
};
// clang-format on