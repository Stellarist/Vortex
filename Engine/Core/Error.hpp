export module Core:Error;

import std;

export namespace Vortex {

enum class ErrorKind {
	Runtime,
	Logic,
	Argument,
	Range,
};

inline constexpr ErrorKind Runtime = ErrorKind::Runtime;
inline constexpr ErrorKind Logic = ErrorKind::Logic;
inline constexpr ErrorKind Argument = ErrorKind::Argument;
inline constexpr ErrorKind Range = ErrorKind::Range;

}        // namespace Vortex


namespace Vortex::Detail {

template <typename... Args>
std::string formatMessage(std::string_view format, Args&&... args)
{
	if constexpr (sizeof...(Args) == 0)
		return std::string(format);
	else
		return std::vformat(format, std::make_format_args(args...));
}

[[noreturn]] void throwError(ErrorKind kind, std::string message)
{
	switch (kind) {
	case ErrorKind::Runtime:
		throw std::runtime_error(std::move(message));
	case ErrorKind::Logic:
		throw std::logic_error(std::move(message));
	case ErrorKind::Argument:
		throw std::invalid_argument(std::move(message));
	case ErrorKind::Range:
		throw std::out_of_range(std::move(message));
	}

	throw std::runtime_error(std::move(message));
}

}        // namespace Vortex::Detail


export namespace Vortex {

template <typename... Args>
[[noreturn]] void ERROR(ErrorKind kind, std::string_view format, Args&&... args)
{
	Detail::throwError(kind, Detail::formatMessage(format, std::forward<Args>(args)...));
}

template <typename... Args>
[[noreturn]] void ERROR(std::string_view format, Args&&... args)
{
	ERROR(Runtime, format, std::forward<Args>(args)...);
}

template <typename Condition, typename... Args>
requires requires(Condition&& value) { static_cast<bool>(value); }
void CHECK(ErrorKind kind, Condition&& expression, std::string_view format, Args&&... args)
{
	if (!static_cast<bool>(expression))
		ERROR(kind, format, std::forward<Args>(args)...);
}

template <typename Condition, typename... Args>
requires requires(Condition&& value) { static_cast<bool>(value); }
void CHECK(Condition&& expression, std::string_view format, Args&&... args)
{
	CHECK(Runtime, std::forward<Condition>(expression), format, std::forward<Args>(args)...);
}

}        // namespace Vortex
