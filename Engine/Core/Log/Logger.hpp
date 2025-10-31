#pragma once

#include <spdlog/spdlog.h>

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
