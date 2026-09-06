module;

#include <yaml-cpp/yaml.h>

module Runtime.Render;

namespace Vortex {

[[noreturn]] static void invalidSetting(const std::filesystem::path& path,
    std::string_view field,
    std::string_view message)
{
	ERROR("Invalid graphics setting '{}' in {}: {}",
	    field,
	    path.string(),
	    message);
}

template <typename T>
static void readScalar(const YAML::Node& root, const std::filesystem::path& path, std::string_view field, T& destination)
{
	const auto node = root[std::string(field)];
	if (!node)
		return;
	if (!node.IsScalar())
		invalidSetting(path, field, "expected a scalar value");
	try {
		destination = node.as<T>();
	} catch (const YAML::Exception& exception) {
		invalidSetting(path, field, exception.what());
	}
}

static void requireFinite(const std::filesystem::path& path, std::string_view field, float value, bool condition, std::string_view range)
{
	if (!std::isfinite(value) || !condition)
		invalidSetting(path, field, range);
}

RenderSettings RenderSettings::load(const std::filesystem::path& path)
{
	YAML::Node root;
	try {
		root = YAML::LoadFile(path.string());
	} catch (const YAML::Exception& exception) {
		ERROR("Failed to load graphics settings from {}: {}",
		    path.string(),
		    exception.what());
	}
	CHECK(root.IsMap(),
	    "Graphics settings must contain a YAML mapping: {}",
	    path.string());

	static const std::unordered_set<std::string> known_fields{
	    "render_path",
	    "frustum_culling",
	    "directional_shadows",
	    "shadow_bias",
	};
	std::unordered_set<std::string> seen_fields;
	for (const auto& entry : root) {
		CHECK(entry.first.IsScalar(),
		    "Graphics setting names must be strings: {}",
		    path.string());
		const auto field = entry.first.Scalar();
		if (!known_fields.contains(field))
			invalidSetting(path, field, "unknown field");
		if (!seen_fields.insert(field).second)
			invalidSetting(path, field, "duplicate field");
	}

	RenderSettings settings;
	if (const auto node = root["render_path"]; node) {
		if (!node.IsScalar())
			invalidSetting(path, "render_path", "expected 'forward' or 'deferred'");
		const auto value = node.Scalar();
		if (value == "forward")
			settings.render_path = RenderPathType::Forward;
		else if (value == "deferred")
			settings.render_path = RenderPathType::Deferred;
		else
			invalidSetting(path, "render_path", "expected 'forward' or 'deferred'");
	}
	readScalar(root, path, "frustum_culling", settings.frustum_culling);
	readScalar(root, path, "directional_shadows", settings.directional_shadows);
	readScalar(root, path, "shadow_bias", settings.shadow_bias);

	requireFinite(path, "shadow_bias", settings.shadow_bias, settings.shadow_bias >= 0.0f && settings.shadow_bias <= 0.05f, "expected a finite value in [0, 0.05]");

	LOG("Loaded graphics settings '{}' (path={})",
	    path.string(),
	    settings.render_path == RenderPathType::Forward ? "forward" : "deferred");
	return settings;
}

}        // namespace Vortex
