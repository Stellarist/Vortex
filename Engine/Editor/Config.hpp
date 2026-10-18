export module Editor:Config;

import Core;
import Runtime.RHI;

export namespace Vortex {

struct Config {
	RHIAPI api{RHIAPI::D3D12};

	std::filesystem::path scene;

	static Config load(const std::filesystem::path& path);
};

}        // namespace Vortex
