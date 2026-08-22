module;

#include <spdlog/sinks/basic_file_sink.h>

module Core.Log;

import Core.File;

namespace Vortex {

std::shared_ptr<spdlog::logger> Logger::logger =
    spdlog::basic_logger_mt("basic_logger", (PathResolver::getLogsDir() / "log.txt").string());

void Logger::info(std::string_view message)
{
	logger->info(message);
}

void Logger::warn(std::string_view message)
{
	logger->warn(message);
}

void Logger::error(std::string_view message)
{
	logger->error(message);
}

void Logger::debug(std::string_view message)
{
	logger->debug(message);
}

}        // namespace Vortex
