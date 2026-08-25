export module Runtime.World:Assets.MeshAsset;

import Core;
import :Assets.Asset;
import :Assets.MaterialAsset;

export namespace Vortex {

struct Vertex {
	Vec3 pos{0.0f};
	Vec3 normal{0.0f, 0.0f, 1.0f};
	Vec2 uv{0.0f};
	Vec4 color{1.0f};
};

struct MeshSection {
	uint32 first_index{};
	uint32 index_count{};
	uint32 first_vertex{};
	uint32 vertex_count{};
	uint32 material_slot{};
};

class MeshAsset : public Asset {
private:
	std::vector<Vertex>                     vertices;
	std::vector<uint32>                     indices;
	std::vector<MeshSection>                sections;
	std::vector<AssetHandle<MaterialAsset>> materials;

public:
	MeshAsset(std::string name, std::string virtual_path = {});
	~MeshAsset() override = default;

	auto getVertices() const noexcept -> const std::vector<Vertex>&;
	auto setVertices(std::vector<Vertex> vertices) -> MeshAsset&;

	auto getIndices() const noexcept -> const std::vector<uint32>&;
	auto setIndices(std::vector<uint32> indices) -> MeshAsset&;

	auto getSections() const noexcept -> const std::vector<MeshSection>&;
	auto setSections(std::vector<MeshSection> sections) -> MeshAsset&;

	auto getMaterials() const noexcept -> const std::vector<AssetHandle<MaterialAsset>>&;
	auto setMaterials(std::vector<AssetHandle<MaterialAsset>> materials) -> MeshAsset&;

	auto getMaterial(uint32 slot) const -> AssetHandle<MaterialAsset>;

	uint32 getVertexCount() const noexcept;
	uint32 getIndexCount() const noexcept;

	bool valid() const noexcept;
};

}        // namespace Vortex
