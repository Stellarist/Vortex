#pragma once

#include <string>

#include "Entity.hpp"

class Node;
class World;
class Scene;

class Behaviour : public Entity {
private:
	std::string name;

	bool started{false};
	bool enabled{true};

protected:
	Node* node{};

public:
	Behaviour() = default;
	Behaviour(std::string name);
	~Behaviour() override = default;

	std::type_index getType() override = 0;

	virtual void start();
	virtual void update(float dt);

	auto getName() const -> const std::string&;
	void setName(const std::string& name);

	auto getNode() const -> Node*;
	void setNode(Node& node);

	bool isStarted() const;
	void setStarted(bool started);

	bool isEnabled() const;
	void setEnabled(bool enabled);

	World* getWorld() const;
	Scene* getScene() const;
};
