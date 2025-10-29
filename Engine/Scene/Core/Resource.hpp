#pragma once

#include "Entity.hpp"

#include <string>

class Resource : public Entity {
private:
	std::string name;
	std::string path;

public:
	Resource() = default;
	Resource(std::string name);

	Resource(const Resource&) = default;
	Resource& operator=(Resource&) = default;

	Resource(Resource&&) noexcept = default;
	Resource& operator=(Resource&&) noexcept = default;

	virtual ~Resource() = default;

	auto getName() const -> const std::string&;
	void setName(const std::string& name);

	auto getPath() const -> const std::string&;
	void setPath(const std::string& path);
};