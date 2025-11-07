#pragma once

#include <string>
#include <typeindex>

#include "Entity.hpp"

class Node;
class World;
class Scene;

class Component : public Entity {
private:
	std::string name;

protected:
	Node* node{};

public:
	Component() = default;
	Component(std::string name);
	~Component() override = default;

	std::type_index getType() override = 0;

	auto getName() const -> const std::string&;
	void setName(const std::string& name);

	Node* getNode() const;
	void  setNode(Node& node);

	World* getWorld() const;
	Scene* getScene() const;
};
