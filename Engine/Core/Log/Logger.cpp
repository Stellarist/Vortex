#include "Logger.hpp"

#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> Logger::logger =
    spdlog::basic_logger_mt("basic_logger", LOGS_DIR "log.txt");

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
