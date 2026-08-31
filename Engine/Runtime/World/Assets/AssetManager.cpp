module Runtime.World;

namespace Vortex {

AssetManager& AssetManager::instance() noexcept
{
	static AssetManager manager{};
	return manager;
}

void AssetManager::unpin(uint64 uid)
{
	pinned_assets.erase(uid);
}

void AssetManager::collectGarbage()
{
	const auto removed_paths = std::erase_if(assets_by_path, [this](const auto& entry) {
		auto loaded_it = loaded_assets.find(entry.second);
		return loaded_it == loaded_assets.end() || loaded_it->second.expired();
	});

	const auto removed_assets = std::erase_if(loaded_assets, [](const auto& entry) {
		return entry.second.expired();
	});

	if (removed_assets != 0 || removed_paths != 0)
		LOG(Debug, "Asset garbage collection removed {} assets and {} path entries",
		    removed_assets, removed_paths);
}

}        // namespace Vortex
