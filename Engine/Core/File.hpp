module;

#include <nlohmann/json.hpp>

export module Core.File;

import std;
import Core.Types;

export namespace Vortex {

class FileSystem {
public:
	static bool    exists(const std::filesystem::path& path);
	static bool    isFile(const std::filesystem::path& path);
	static bool    isDirectory(const std::filesystem::path& path);
	static uintmax getFileSize(const std::filesystem::path& path);

	static bool createDirectory(const std::filesystem::path& path);
	static bool createDirectories(const std::filesystem::path& path);
	static bool removeDirectory(const std::filesystem::path& path);

	static bool copyFile(const std::filesystem::path& src, const std::filesystem::path& dst, bool overwrite = false);
	static bool moveFile(const std::filesystem::path& src, const std::filesystem::path& dst);
	static bool deleteFile(const std::filesystem::path& path);

	static std::string            readTextFile(const std::filesystem::path& path);
	static std::vector<std::byte> readBinaryFile(const std::filesystem::path& path);

	static bool writeTextFile(const std::filesystem::path& path, const std::string& content);
	static bool writeBinaryFile(const std::filesystem::path& path, const std::vector<std::byte>& data);
};


class JsonParser {
public:
	static nlohmann::json readJson(const std::filesystem::path& path);
	static bool           writeJson(const std::filesystem::path& path, const nlohmann::json& data, int indent);
};


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

}        // namespace Vortex
