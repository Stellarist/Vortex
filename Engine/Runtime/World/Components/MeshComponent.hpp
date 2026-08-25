export module Runtime.World:Components.MeshComponent;

import Core;
import :Assets.MeshAsset;
import :Components.PrimitiveComponent;

export namespace Vortex {

class MeshComponent : public PrimitiveComponent {
private:
	AssetHandle<MeshAsset>                  mesh;
	std::vector<AssetHandle<MaterialAsset>> materials;

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
};

}        // namespace Vortex
