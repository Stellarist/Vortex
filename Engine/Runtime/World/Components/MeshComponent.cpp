module Runtime.World;

namespace Vortex {

MeshComponent::MeshComponent(std::string component_name) :
    PrimitiveComponent(std::move(component_name))
{}

const AssetHandle<MeshAsset>& MeshComponent::getMesh() const noexcept
{
	return mesh;
}

MeshComponent& MeshComponent::setMesh(AssetHandle<MeshAsset> new_mesh)
{
	mesh = std::move(new_mesh);
	materials.clear();
	return *this;
}

MeshComponent& MeshComponent::clearMesh() noexcept
{
	mesh.reset();
	materials.clear();
	return *this;
}

AssetHandle<MaterialAsset> MeshComponent::getMaterial(uint32 slot) const
{
	if (slot < materials.size() && materials[slot])
		return materials[slot];
	return mesh ? mesh->getMaterial(slot) : AssetHandle<MaterialAsset>{};
}

MeshComponent& MeshComponent::setMaterial(uint32 slot, AssetHandle<MaterialAsset> material)
{
	if (materials.size() <= slot)
		materials.resize(static_cast<size_t>(slot) + 1);
	materials[slot] = std::move(material);
	return *this;
}

MeshComponent& MeshComponent::clearMaterial(uint32 slot) noexcept
{
	if (slot < materials.size())
		materials[slot].reset();
	return *this;
}

MeshComponent& MeshComponent::clearMaterials() noexcept
{
	materials.clear();
	return *this;
}

}        // namespace Vortex
