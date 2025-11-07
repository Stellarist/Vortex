#pragma once

#include <string>
#include <filesystem>

class PathResolver {
public:
	static std::filesystem::path getExecutableDir();

	static std::filesystem::path getAssetsDir();
	static std::filesystem::path getConfigsDir();
	static std::filesystem::path getShadersDir();
	static std::filesystem::path getScriptsDir();
	static std::filesystem::path getLogsDir();

	static std::filesystem::path resolveAssetPath(const std::string& relativePath);
};
