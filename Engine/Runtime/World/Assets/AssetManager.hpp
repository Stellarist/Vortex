export module Runtime.World:Assets.AssetManager;

import Core;
import :Assets.Asset;

export namespace Vortex {

class AssetManager {
private:
	std::unordered_map<uint64, std::weak_ptr<Asset>> loaded_assets;
	std::unordered_map<uint64, std::shared_ptr<Asset>> pinned_assets;
	std::unordered_map<std::string, uint64> assets_by_path;

	AssetManager() = default;
	~AssetManager() = default;

	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;

	AssetManager(AssetManager&&) noexcept = delete;
	AssetManager& operator=(AssetManager&&) noexcept = delete;

public:
	static AssetManager& instance() noexcept;

	template <IsAsset T>
	AssetHandle<T> add(std::shared_ptr<T> asset);

	template <IsAsset T>
	AssetHandle<T> findByPath(std::string_view virtual_path) const;

	template <IsAsset T>
	auto getLoadedAssets() const -> std::vector<AssetHandle<T>>;

	template <IsAsset T>
	void pin(const AssetHandle<T>& asset);
	void unpin(uint64 uid);

	void collectGarbage();
};

template <IsAsset T>
AssetHandle<T> AssetManager::add(std::shared_ptr<T> asset)
{
	CHECK(Argument, asset, "Cannot add an empty asset");

	const auto& virtual_path = asset->getVirtualPath();
	if (!virtual_path.empty()) {
		if (auto path_it = assets_by_path.find(virtual_path); path_it != assets_by_path.end()) {
			if (auto loaded_it = loaded_assets.find(path_it->second); loaded_it != loaded_assets.end()) {
				if (auto existing = loaded_it->second.lock()) {
					auto typed_existing = std::dynamic_pointer_cast<T>(existing);
					CHECK(typed_existing, "An asset path cannot refer to multiple asset types");
					return AssetHandle<T>(std::move(typed_existing));
				}
			}
			assets_by_path.erase(path_it);
		}
		assets_by_path.insert_or_assign(virtual_path, asset->getUid());
	}

	loaded_assets.insert_or_assign(asset->getUid(), asset);
	return AssetHandle<T>(std::move(asset));
}

template <IsAsset T>
AssetHandle<T> AssetManager::findByPath(std::string_view virtual_path) const
{
	auto path_it = assets_by_path.find(std::string(virtual_path));
	if (path_it == assets_by_path.end())
		return {};

	auto loaded_it = loaded_assets.find(path_it->second);
	if (loaded_it == loaded_assets.end())
		return {};

	auto asset = std::dynamic_pointer_cast<T>(loaded_it->second.lock());
	return asset ? AssetHandle<T>(std::move(asset)) : AssetHandle<T>{};
}

template <IsAsset T>
std::vector<AssetHandle<T>> AssetManager::getLoadedAssets() const
{
	std::vector<AssetHandle<T>> result;
	for (const auto& [uid, weak_asset] : loaded_assets)
		if (auto asset = std::dynamic_pointer_cast<T>(weak_asset.lock()))
			result.push_back(AssetHandle<T>(std::move(asset)));
	return result;
}

template <IsAsset T>
void AssetManager::pin(const AssetHandle<T>& asset)
{
	if (asset)
		pinned_assets.insert_or_assign(asset->getUid(), asset.asset);
}

}        // namespace Vortex
