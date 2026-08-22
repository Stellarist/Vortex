module;

#include <spdlog/spdlog.h>

export module Core.Log;

import std;

export namespace Vortex {

class Logger {
private:
	static std::shared_ptr<spdlog::logger> logger;

	Logger() = delete;

public:
	static void info(std::string_view message);
	static void warn(std::string_view message);
	static void error(std::string_view message);
	static void debug(std::string_view message);
};

}        // namespace Vortex
