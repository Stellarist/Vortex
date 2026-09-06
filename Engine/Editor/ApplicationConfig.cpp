module;

#include <yaml-cpp/yaml.h>

module Editor;

namespace Vortex {

ApplicationConfig ApplicationConfig::load(const std::filesystem::path& path)
{
	YAML::Node document;
	try {
		document = YAML::LoadFile(path.string());
	} catch (const YAML::Exception& exception) {
		ERROR("Failed to load application config from {}: {}",
		    path.string(),
		    exception.what());
	}

	CHECK(document.IsMap(),
	    "Application config must contain a YAML mapping: {}",
	    path.string());

	bool scene_seen{};
	for (const auto& entry : document) {
		CHECK(entry.first.IsScalar(),
		    "Application config field names must be strings: {}",
		    path.string());

		const auto field = entry.first.Scalar();
		CHECK(field == "scene",
		    "Unknown application config field '{}' in {}",
		    field,
		    path.string());
		CHECK(!scene_seen,
		    "Application config contains duplicate 'scene' fields: {}",
		    path.string());
		scene_seen = true;
	}

	const auto scene = document["scene"];
	CHECK(scene_seen && scene.IsScalar(),
	    "Application config requires a scalar 'scene' field: {}",
	    path.string());

	std::filesystem::path scene_path(scene.Scalar());
	CHECK(!scene_path.empty() && !scene_path.is_absolute(),
	    "Application scene path must be a non-empty relative path: {}",
	    path.string());

	scene_path = scene_path.lexically_normal();
	for (const auto& part : scene_path)
		CHECK(part != "..",
		    "Application scene path cannot leave the Assets directory: {}",
		    path.string());

	LOG("Loaded application config '{}' with scene '{}'",
	    path.string(),
	    scene_path.string());
	return ApplicationConfig{.scene = std::move(scene_path)};
}

}        // namespace Vortex
