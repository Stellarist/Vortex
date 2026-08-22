export module Runtime.World:SubMesh;

import Core;
import :Resource;
import :Material;
import :AABB;

export namespace Vortex {

struct Vertex {
	Vec3 pos{0.0f};
	Vec3 normal{0.0f, 0.0f, 1.0f};
	Vec2 uv{0.0f};
	Vec4 color{1.0f};
};

class SubMesh : public Resource {
private:
	std::vector<Vertex> vertex_data;
	std::vector<uint32> index_data;

	std::shared_ptr<Material> material{};

	std::string shader_name;

	bool visible{true};

	mutable AABB aabb{};
	mutable bool aabb_dirty{true};

	void updateAABB() const;

public:
	SubMesh(const std::string& name = {});
	~SubMesh() override = default;

	std::type_index getType() override;

	auto getVertices() const -> const std::vector<Vertex>&;
	void setVertices(std::vector<Vertex> vertex_data);

	auto getIndices() const -> const std::vector<uint32>&;
	void setIndices(std::vector<uint32> index_data);

	uint32 getVerticesCount() const;
	uint32 getIndicesCount() const;

	auto getMaterial() const -> std::shared_ptr<Material>;
	void setMaterial(std::shared_ptr<Material> material);

	auto getShaderName() const -> const std::string&;
	void setShaderName(const std::string& shader_name);

	bool isVisible() const;
	void setVisible(bool visible);

	auto getAABB() const -> const AABB&;
	void invalidateAABB() const;
};

}        // namespace Vortex
