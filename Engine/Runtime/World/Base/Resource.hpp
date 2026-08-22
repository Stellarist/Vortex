export module Runtime.World:Resource;

import Core;
import :Entity;

export namespace Vortex {

class Resource : public Entity {
private:
	std::string name;

public:
	Resource() = default;
	Resource(std::string name);
	virtual ~Resource() = default;

	auto getName() const -> const std::string&;
	void setName(const std::string& name);
};

}        // namespace Vortex
