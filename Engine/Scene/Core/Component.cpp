#include "Component.hpp"

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
