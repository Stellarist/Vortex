export module Runtime.World:Mesh;

import Core;
import :Component;
import :SubMesh;

export namespace Vortex {

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

}        // namespace Vortex
