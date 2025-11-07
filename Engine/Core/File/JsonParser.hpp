#pragma once

#include <nlohmann/json.hpp>

class JsonParser {
public:
	static nlohmann::json readJson(const std::filesystem::path& path);
	static bool           writeJson(const std::filesystem::path& path, const nlohmann::json& data, int indent);
};
