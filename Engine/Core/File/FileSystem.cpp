#include "FileSystem.hpp"

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
