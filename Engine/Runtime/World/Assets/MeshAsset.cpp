module Runtime.Asset;

namespace Vortex {

MeshAsset::MeshAsset(std::string asset_name, std::string asset_path) :
    Asset(std::move(asset_name), std::move(asset_path))
{}

const std::vector<Vertex>& MeshAsset::getVertices() const noexcept
{
	return vertices;
}

MeshAsset& MeshAsset::setVertices(std::vector<Vertex> new_vertices)
{
	vertices = std::move(new_vertices);
	touch();
	return *this;
}

const std::vector<uint32>& MeshAsset::getIndices() const noexcept
{
	return indices;
}

MeshAsset& MeshAsset::setIndices(std::vector<uint32> new_indices)
{
	indices = std::move(new_indices);
	touch();
	return *this;
}

const std::vector<MeshSection>& MeshAsset::getSections() const noexcept
{
	return sections;
}

MeshAsset& MeshAsset::setSections(std::vector<MeshSection> new_sections)
{
	sections = std::move(new_sections);
	touch();
	return *this;
}

const std::vector<AssetHandle<MaterialAsset>>& MeshAsset::getMaterials() const noexcept
{
	return materials;
}

MeshAsset& MeshAsset::setMaterials(std::vector<AssetHandle<MaterialAsset>> new_materials)
{
	materials = std::move(new_materials);
	touch();
	return *this;
}

AssetHandle<MaterialAsset> MeshAsset::getMaterial(uint32 slot) const
{
	return slot < materials.size() ? materials[slot] : AssetHandle<MaterialAsset>{};
}

uint32 MeshAsset::getVertexCount() const noexcept
{
	return static_cast<uint32>(vertices.size());
}

uint32 MeshAsset::getIndexCount() const noexcept
{
	return static_cast<uint32>(indices.size());
}

bool MeshAsset::valid() const noexcept
{
	return !vertices.empty() && !indices.empty() && !sections.empty();
}

}        // namespace Vortex
