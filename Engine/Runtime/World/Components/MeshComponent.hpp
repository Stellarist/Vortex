export module Runtime.World:Components.MeshComponent;

import Core;
import :Assets.MeshAsset;
import :Components.SceneComponent;

export namespace Vortex {

class MeshComponent : public SceneComponent {
private:
	AssetHandle<MeshAsset>                  mesh;
	std::vector<AssetHandle<MaterialAsset>> materials;

	bool visible{true};
	bool casts_shadow{true};

public:
	MeshComponent(std::string name);
	~MeshComponent() override = default;

	auto getMesh() const noexcept -> const AssetHandle<MeshAsset>&;
	auto setMesh(AssetHandle<MeshAsset> mesh) -> MeshComponent&;
	auto clearMesh() noexcept -> MeshComponent&;

	auto getMaterial(uint32 slot) const -> AssetHandle<MaterialAsset>;
	auto setMaterial(uint32 slot, AssetHandle<MaterialAsset> material) -> MeshComponent&;

	auto clearMaterial(uint32 slot) noexcept -> MeshComponent&;
	auto clearMaterials() noexcept -> MeshComponent&;

	bool isVisible() const noexcept;
	auto setVisible(bool visible) noexcept -> MeshComponent&;

	bool castsShadow() const noexcept;
	auto setCastsShadow(bool casts_shadow) noexcept -> MeshComponent&;
};

}        // namespace Vortex
