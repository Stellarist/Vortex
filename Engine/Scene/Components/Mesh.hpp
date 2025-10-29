#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Scene/Core/Component.hpp"
#include "Scene/Resources/SubMesh.hpp"

class Mesh : public Component {
private:
	std::vector<std::shared_ptr<SubMesh>> submeshes;

public:
	Mesh(const std::string& name);

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	~Mesh() override = default;

	std::type_index getType() override;

	auto getSubmeshes() const -> const std::vector<std::shared_ptr<SubMesh>>;
	void addSubmesh(std::shared_ptr<SubMesh> submesh);
};
