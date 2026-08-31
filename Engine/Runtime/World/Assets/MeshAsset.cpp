module Runtime.World;

namespace Vortex {

MeshAsset::MeshAsset(std::string asset_name, std::string asset_path) :
    Asset(std::move(asset_name), std::move(asset_path))
{}

const std::vector<MeshVertex>& MeshAsset::getVertices() const noexcept
{
	return vertices;
}

MeshAsset& MeshAsset::setVertices(std::vector<MeshVertex> new_vertices)
{
	vertices = std::move(new_vertices);
	local_bounds.reset();
	for (const auto& vertex : vertices)
		if (std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y) && std::isfinite(vertex.pos.z))
			local_bounds.expand(vertex.pos);

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

const Bounds& MeshAsset::getLocalBounds() const noexcept
{
	return local_bounds;
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
	if (vertices.empty() || indices.empty() || sections.empty() || materials.empty() || !local_bounds.valid())
		return false;

	if (std::ranges::any_of(vertices, [](const MeshVertex& vertex) {
		    return !std::isfinite(vertex.pos.x) || !std::isfinite(vertex.pos.y) || !std::isfinite(vertex.pos.z);
	    }))
		return false;

	for (const auto& section : sections) {
		if (section.vertex_count == 0 || section.index_count == 0 ||
		    section.first_vertex > vertices.size() || section.vertex_count > vertices.size() - section.first_vertex ||
		    section.first_index > indices.size() || section.index_count > indices.size() - section.first_index ||
		    section.material_slot >= materials.size() || !materials[section.material_slot])
			return false;

		const auto section_indices = std::span(indices).subspan(section.first_index, section.index_count);
		if (std::ranges::any_of(section_indices, [&section](uint32 index) { return index >= section.vertex_count; }))
			return false;
	}
	return true;
}

}        // namespace Vortex
