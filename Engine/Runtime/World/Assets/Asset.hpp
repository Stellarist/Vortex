export module Runtime.Asset:Asset;

import Core;
export import Runtime.Object;

export namespace Vortex {

class Asset : public Object {
private:
	std::string virtual_path;
	uint64      revision{};

protected:
	void touch() noexcept;

public:
	Asset(std::string name, std::string virtual_path = {});
	virtual ~Asset() noexcept = default;

	Asset(const Asset&) = delete;
	Asset& operator=(const Asset&) = delete;
	Asset(Asset&&) noexcept = delete;
	Asset& operator=(Asset&&) noexcept = delete;

	auto   getVirtualPath() const noexcept -> const std::string&;
	uint64 getRevision() const noexcept;
};

template <typename T>
concept IsAsset = std::derived_from<T, Asset>;


template <IsAsset T>
class AssetHandle {
private:
	std::shared_ptr<T> asset;

	AssetHandle(std::shared_ptr<T> asset) :
	    asset(std::move(asset)) {}

	friend class AssetManager;

public:
	AssetHandle() noexcept = default;

	T* get() const noexcept { return asset.get(); }
	T& operator*() const { return *asset; }
	T* operator->() const noexcept { return asset.get(); }

	operator bool() const noexcept { return asset != nullptr; }

	void reset() noexcept { asset.reset(); }

	bool operator==(const AssetHandle& other) const noexcept { return asset == other.asset; }
};

}        // namespace Vortex
