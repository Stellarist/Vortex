export module Core:Log;

import std;
import :File;

export namespace Vortex {

enum class LogLevel {
	Debug,
	Info,
	Warn,
	Error,
};

inline constexpr LogLevel Debug = LogLevel::Debug;
inline constexpr LogLevel Info = LogLevel::Info;
inline constexpr LogLevel Warn = LogLevel::Warn;
inline constexpr LogLevel Error = LogLevel::Error;

}        // namespace Vortex


namespace Vortex::Detail {

std::mutex& mutex()
{
	static std::mutex mutex;
	return mutex;
}

std::ofstream& file()
{
	static std::ofstream file = [] {
		std::ofstream result;
		try {
			result.open(File::logsDir() / "log.txt", std::ios::out | std::ios::trunc);
		} catch (...) {
		}
		return result;
	}();
	return file;
}

std::string_view name(LogLevel level)
{
	switch (level) {
	case LogLevel::Debug:
		return "Debug";
	case LogLevel::Info:
		return "Info";
	case LogLevel::Warn:
		return "Warning";
	case LogLevel::Error:
		return "Error";
	}

	return "Info";
}

void write(LogLevel level, std::string_view message)
{
	const bool flush = level == LogLevel::Error;
	const std::scoped_lock lock(mutex());

#if VDEBUG
	std::clog << '[' << name(level) << "] " << message << '\n';
	if (flush)
		std::clog.flush();
#endif

	auto& log_file = file();
	if (!log_file)
		return;

	log_file << '[' << name(level) << "] " << message << '\n';
	if (flush)
		log_file.flush();
}

template <typename... Args>
void writeFormatted(LogLevel level, std::string_view format, Args&&... args)
{
	if constexpr (sizeof...(Args) == 0)
		write(level, format);
	else
		write(level, std::vformat(format, std::make_format_args(args...)));
}

}        // namespace Vortex::Detail


export namespace Vortex {

template <typename... Args>
void LOG(LogLevel level, std::string_view format, Args&&... args)
{
	Detail::writeFormatted(level, format, std::forward<Args>(args)...);
}

template <typename... Args>
void LOG(std::string_view format, Args&&... args)
{
	LOG(Info, format, std::forward<Args>(args)...);
}

void flushLog()
{
	const std::scoped_lock lock(Detail::mutex());

#if VDEBUG
	std::clog.flush();
#endif
	if (auto& log_file = Detail::file(); log_file)
		log_file.flush();
}

}        // namespace Vortex
