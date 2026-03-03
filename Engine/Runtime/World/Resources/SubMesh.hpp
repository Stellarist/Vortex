#pragma once

#include <string>
#include <vector>

#include "Material.hpp"
#include "Runtime/World/Geometry/AABB.hpp"

struct Vertex {
	glm::vec3 pos{0.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec2 uv{0.0f};
	glm::vec4 color{1.0f};
};

class SubMesh : public Resource {
private:
	std::vector<Vertex>   vertex_data;
	std::vector<uint32_t> index_data;

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

	auto getIndices() const -> const std::vector<uint32_t>&;
	void setIndices(std::vector<uint32_t> index_data);

	uint32_t getVerticesCount() const;
	uint32_t getIndicesCount() const;

	auto getMaterial() const -> std::shared_ptr<Material>;
	void setMaterial(std::shared_ptr<Material> material);

	auto getShaderName() const -> const std::string&;
	void setShaderName(const std::string& shader_name);

	bool isVisible() const;
	void setVisible(bool visible);

	auto getAABB() const -> const AABB&;
	void invalidateAABB() const;
};
