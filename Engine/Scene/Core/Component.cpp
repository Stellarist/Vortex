#include "Component.hpp"

#include "Scene/Core/Node.hpp"

Component::Component(std::string name) :
    name(std::move(name))
{}

const std::string& Component::getName() const
{
	return name;
}

void Component::setName(const std::string& name)
{
	this->name = name;
}

void Component::setNode(Node& node)
{
	this->node = &node;
}

Node* Component::getNode() const
{
	return node;
}

World* Component::getWorld() const
{
	if (node)
		return node->getWorld();

	return nullptr;
}

Scene* Component::getScene() const
{
	if (node)
		return node->getScene();

	return nullptr;
}
