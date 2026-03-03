#include "File.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

bool FileSystem::exists(const std::filesystem::path& path)
{
	return std::filesystem::exists(path);
}

bool FileSystem::isFile(const std::filesystem::path& path)
{
	return std::filesystem::is_regular_file(path);
}

bool FileSystem::isDirectory(const std::filesystem::path& path)
{
	return std::filesystem::is_directory(path);
}

std::uintmax_t FileSystem::getFileSize(const std::filesystem::path& path)
{
	if (!exists(path))
		throw std::runtime_error("File does not exist: " + path.string());

	return std::filesystem::file_size(path);
}

bool FileSystem::createDirectory(const std::filesystem::path& path)
{
	return std::filesystem::create_directory(path);
}

bool FileSystem::createDirectories(const std::filesystem::path& path)
{
	return std::filesystem::create_directories(path);
}

bool FileSystem::removeDirectory(const std::filesystem::path& path)
{
	if (!exists(path))
		return false;

	return std::filesystem::remove_all(path) > 0;
}

bool FileSystem::copyFile(const std::filesystem::path& src, const std::filesystem::path& dst, bool overwrite)
{
	if (!exists(src))
		return false;

	auto options = overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;

	try {
		std::filesystem::copy_file(src, dst, options);
		return true;
	} catch (...) {
		return false;
	}
}

bool FileSystem::moveFile(const std::filesystem::path& src, const std::filesystem::path& dst)
{
	if (!exists(src))
		return false;

	try {
		std::filesystem::rename(src, dst);
		return true;
	} catch (...) {
		return false;
	}
}

bool FileSystem::deleteFile(const std::filesystem::path& path)
{
	if (!exists(path))
		return false;

	return std::filesystem::remove(path);
}

std::string FileSystem::readTextFile(const std::filesystem::path& path)
{
	if (!exists(path))
		throw std::runtime_error("File does not exist: " + path.string());

	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error("Failed to open file: " + path.string());

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::vector<uint8_t> FileSystem::readBinaryFile(const std::filesystem::path& path)
{
	if (!exists(path))
		throw std::runtime_error("File does not exist: " + path.string());

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
		throw std::runtime_error("Failed to open file: " + path.string());

	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	file.read(reinterpret_cast<char*>(buffer.data()), size);

	return buffer;
}

bool FileSystem::writeTextFile(const std::filesystem::path& path, const std::string& content)
{
	// Create parent directories if they don't exist
	auto parentPath = path.parent_path();
	if (!parentPath.empty() && !exists(parentPath))
		createDirectories(parentPath);

	std::ofstream file(path);
	if (!file.is_open())
		return false;

	file << content;
	return file.good();
}

bool FileSystem::writeBinaryFile(const std::filesystem::path& path, const std::vector<uint8_t>& data)
{
	auto parentPath = path.parent_path();
	if (!parentPath.empty() && !exists(parentPath))
		createDirectories(parentPath);

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
		return false;

	file.write(reinterpret_cast<const char*>(data.data()), data.size());
	return file.good();
}

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

std::filesystem::path PathResolver::getExecutableDir()
{
	return std::filesystem::current_path();
}

std::filesystem::path PathResolver::getAssetsDir()
{
	if (auto dir = getExecutableDir() / "Assets"; FileSystem::exists(dir))
		return dir;

	throw std::runtime_error("Assets directory not found");
}

std::filesystem::path PathResolver::getConfigsDir()
{
	if (auto dir = getExecutableDir() / "Configs"; FileSystem::exists(dir))
		return dir;

	throw std::runtime_error("Configs directory not found");
}

std::filesystem::path PathResolver::getShadersDir()
{
	if (auto dir = getExecutableDir() / "Shaders"; FileSystem::exists(dir))
		return dir;

	throw std::runtime_error("Shaders directory not found");
}

std::filesystem::path PathResolver::getScriptsDir()
{
	if (auto dir = getExecutableDir() / "Scripts"; FileSystem::exists(dir))
		return dir;

	throw std::runtime_error("Scripts directory not found");
}

std::filesystem::path PathResolver::getLogsDir()
{
	auto dir = getExecutableDir() / "Logs";
	if (!FileSystem::exists(dir))
		FileSystem::createDirectories(dir);

	return dir;
}

std::filesystem::path PathResolver::resolveAssetPath(const std::string& relativePath)
{
	return getAssetsDir() / relativePath;
}
