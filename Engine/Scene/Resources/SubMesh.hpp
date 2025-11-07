#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Material.hpp"

struct VertexAttribute {
	uint32_t size = 0;
	uint32_t offset = 0;
};

class SubMesh : public Resource {
private:
	std::shared_ptr<Material> material{};

	std::string shader_name;
	uint32_t    vertices_count{0};
	uint32_t    indices_count{0};

	std::vector<float>    vertex_data;
	std::vector<uint32_t> index_data;

	std::unordered_map<std::string, VertexAttribute> vertex_attributes;

	bool visible{true};

public:
	SubMesh(const std::string& name = {});
	~SubMesh() override = default;

	std::type_index getType() override;

	uint32_t getVerticesCount() const;
	uint32_t getIndicesCount() const;

	auto getVertices() const -> const std::vector<float>&;
	void setVertices(std::vector<float> vertex_data, uint32_t count = 0);

	auto getIndices() const -> const std::vector<uint32_t>&;
	void setIndices(std::vector<uint32_t> index_data);

	auto getAttributes() const -> const std::unordered_map<std::string, VertexAttribute>&;
	auto getAttribute(const std::string& name) -> VertexAttribute*;
	auto getAttribute(const std::string& name) const -> const VertexAttribute*;
	void setAttribute(const std::string& name, const VertexAttribute& attribute);

	auto getMaterial() const -> std::shared_ptr<Material>;
	void setMaterial(std::shared_ptr<Material> material);

	auto getShaderName() const -> const std::string&;
	void setShaderName(const std::string& shader_name);

	bool isVisible() const;
	void setVisible(bool visible);
};
