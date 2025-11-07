#include "JsonParser.hpp"

#include <fstream>

#include "FileSystem.hpp"

nlohmann::json JsonParser::readJson(const std::filesystem::path& path)
{
	if (!FileSystem::exists(path))
		throw std::runtime_error("JSON file does not exist: " + path.string());

	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error("Failed to open JSON file: " + path.string());

	try {
		nlohmann::json jsonData;
		file >> jsonData;
		return jsonData;
	} catch (const nlohmann::json::parse_error& e) {
		throw std::runtime_error("JSON parse error in file " + path.string() + ": " + e.what());
	}
}

bool JsonParser::writeJson(const std::filesystem::path& path, const nlohmann::json& data, int indent)
{
	try {
		std::string jsonString = data.dump(indent);
		return FileSystem::writeTextFile(path, jsonString);
	} catch (...) {
		return false;
	}
}
