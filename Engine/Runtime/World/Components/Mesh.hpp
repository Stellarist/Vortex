#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Runtime/World/Base/Component.hpp"
#include "Runtime/World/Resources/SubMesh.hpp"

class Mesh : public Component {
private:
	std::vector<std::shared_ptr<SubMesh>> submeshes;

public:
	Mesh(const std::string& name);
	~Mesh() override = default;

	std::type_index getType() override;

	auto getSubmeshes() const -> const std::vector<std::shared_ptr<SubMesh>>;
	void addSubmesh(std::shared_ptr<SubMesh> submesh);
};
