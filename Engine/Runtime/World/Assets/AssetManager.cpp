module Runtime.World;

namespace Vortex {

void AssetManager::unpin(uint64 uid)
{
	pinned_assets.erase(uid);
}

void AssetManager::collectGarbage()
{
	std::erase_if(assets_by_path, [this](const auto& entry) {
		auto loaded_it = loaded_assets.find(entry.second);
		return loaded_it == loaded_assets.end() || loaded_it->second.expired();
	});

	std::erase_if(loaded_assets, [](const auto& entry) {
		return entry.second.expired();
	});
}

}        // namespace Vortex
