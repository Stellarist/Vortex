export module Editor:ApplicationConfig;

import Core;

export namespace Vortex {

struct ApplicationConfig {
	std::filesystem::path scene;

	static ApplicationConfig load(const std::filesystem::path& path);
};

}        // namespace Vortex
