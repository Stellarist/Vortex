export module Core:File;

import std;
import :Error;

namespace Vortex::File {

static std::optional<std::filesystem::path> executable_directory;

static std::filesystem::path requireDirectory(std::string_view name);

}        // namespace Vortex::File


export namespace Vortex::File {

std::vector<std::byte> readTextFile(const std::filesystem::path& path)
{
	std::ifstream file(path);
	CHECK(file, "Failed to open text file: {}", path.string());

	std::ostringstream stream;
	stream << file.rdbuf();
	CHECK(!file.bad(), "Failed to read complete text file: {}", path.string());

	const auto text = std::move(stream).str();
	const auto bytes = std::as_bytes(std::span{text});
	return {bytes.begin(), bytes.end()};
}

std::vector<std::byte> readBinaryFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	CHECK(file, "Failed to open binary file: {}", path.string());

	const auto end_position = file.tellg();
	CHECK(end_position >= 0, "Failed to determine binary file size: {}", path.string());

	const auto size = static_cast<size_t>(end_position);
	file.seekg(0, std::ios::beg);
	CHECK(file, "Failed to seek binary file: {}", path.string());

	std::vector<std::byte> data(size);
	CHECK(size == 0 || file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size)),
	    "Failed to read complete binary file: {}", path.string());
	return data;
}

void initialize(const std::filesystem::path& executable_path)
{
	std::error_code error;
	CHECK(Argument, !executable_path.empty(), "File requires argv[0] or another executable path");

	auto resolved_path = std::filesystem::absolute(executable_path, error);
	CHECK(!static_cast<bool>(error), "Failed to resolve executable path: {}", error.message());

	auto canonical_path = std::filesystem::weakly_canonical(resolved_path, error);
	if (!error)
		resolved_path = std::move(canonical_path);

	executable_directory = std::filesystem::is_directory(resolved_path) ?
	    resolved_path :
	    resolved_path.parent_path();
	CHECK(!executable_directory->empty(), "Executable directory is empty");
}

std::filesystem::path executableDir()
{
	CHECK(executable_directory, "File must be initialized by the program entry point");
	return *executable_directory;
}

std::filesystem::path assetsDir()
{
	return requireDirectory("Assets");
}

std::filesystem::path configsDir()
{
	return requireDirectory("Configs");
}

std::filesystem::path shadersDir()
{
	return requireDirectory("Shaders");
}

std::filesystem::path logsDir()
{
	auto directory = executableDir() / "Logs";

	std::error_code error;
	std::filesystem::create_directories(directory, error);
	CHECK(!static_cast<bool>(error), "Failed to create Logs directory: {}", error.message());
	return directory;
}

}        // namespace Vortex::File

namespace Vortex::File {

std::filesystem::path requireDirectory(std::string_view name)
{
	auto directory = executableDir() / name;
	CHECK(std::filesystem::is_directory(directory), "{} directory not found: {}", name, directory.string());
	return directory;
}

}        // namespace Vortex::File
