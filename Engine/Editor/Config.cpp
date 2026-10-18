module;

#include <toml++/toml.hpp>

module Editor;

namespace Vortex {

Config Config::load(const std::filesystem::path& path)
{
	toml::table document;
	try {
		document = toml::parse_file(path.string());
	} catch (const toml::parse_error& exception) {
		ERROR("Failed to load application config from {}: {}", path.string(), exception.description());
	}

	Config config{};
	for (const auto& [key, node] : document) {
		const auto field = key.str();
		if (field == "api") {
			const auto value = node.value<std::string>();
			CHECK(value, "Application 'api' must be a string: vulkan or d3d12");
			if (*value == "d3d12")
				config.api = RHIAPI::D3D12;
			else if (*value == "vulkan")
				config.api = RHIAPI::Vulkan;
			else
				ERROR("Unknown rendering API '{}'", *value);

		} else if (field == "scene") {
			const auto value = node.value<std::string>();
			CHECK(value, "Application 'scene' must be a string: {}", path.string());
			config.scene = *value;

		} else {
			ERROR("Unknown application config field '{}' in {}", field, path.string());
		}
	}

	CHECK(!config.scene.empty() && !config.scene.is_absolute(),
	    "Application scene path must be a non-empty relative path: {}", path.string());

	config.scene = config.scene.lexically_normal();
	for (const auto& part : config.scene)
		CHECK(part != "..", "Application scene path cannot leave the Assets directory: {}", path.string());
	LOG("Loaded application config '{}' with scene '{}'", path.string(), config.scene.string());

	return config;
}

}        // namespace Vortex
