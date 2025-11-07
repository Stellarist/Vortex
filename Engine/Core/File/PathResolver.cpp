#include "PathResolver.hpp"
#include "FileSystem.hpp"

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
