#include "Behaviour.hpp"

Behaviour::Behaviour(std::string name) :
    name(std::move(name))
{}

void Behaviour::start()
{}

void Behaviour::update(float dt)
{}

const std::string& Behaviour::getName() const
{
	return name;
}

void Behaviour::setName(const std::string& name)
{
	this->name = name;
}

Node* Behaviour::getNode() const
{
	return node;
}

void Behaviour::setNode(Node& node)
{
	this->node = &node;
}

bool Behaviour::isStarted() const
{
	return started;
}

void Behaviour::setStarted(bool started)
{
	this->started = started;
}

bool Behaviour::isEnabled() const
{
	return enabled;
}

void Behaviour::setEnabled(bool enabled)
{
	this->enabled = enabled;
}
