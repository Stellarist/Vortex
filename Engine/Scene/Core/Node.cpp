#include "Node.hpp"

#include "Scene.hpp"

Node::Node(size_t id, std::string name) :
    id(id), name(std::move(name))
{
	transform.setNode(*this);
	setComponent(transform);
}

std::type_index Node::getType()
{
	return typeid(Node);
}

size_t Node::getId() const
{
	return id;
}

const std::string& Node::getName() const
{
	return name;
}

void Node::setName(const std::string& name)
{
	this->name = name;
}

Node* Node::getParent() const
{
	return parent;
}

void Node::setParent(Node& parent)
{
	this->parent = &parent;
	transform.invalidateWorldMatrix();
}

Transform& Node::getTransform()
{
	return transform;
}

Scene* Node::getScene() const
{
	return scene;
}

void Node::setScene(Scene& scene)
{
	this->scene = &scene;
}

World* Node::getWorld() const
{
	return scene ? scene->getWorld() : world;
}

Component& Node::getComponent(std::type_index type) const
{
	return *components.at(type);
}

void Node::setComponent(Component& component)
{
	components[component.getType()] = &component;
}

bool Node::hasComponent(std::type_index type) const
{
	return components.count(type) > 0;
}

void Node::addBehaviour(Behaviour& behaviour)
{
	behaviours.push_back(&behaviour);
	behaviour.setNode(*this);
}

void Node::removeBehaviour(Behaviour& behaviour)
{
	behaviours.erase(
	    std::remove(
	        behaviours.begin(), behaviours.end(), &behaviour),
	    behaviours.end());
}

const std::vector<Node*>& Node::getChildren() const
{
	return children;
}

void Node::addChild(Node& child)
{
	children.push_back(&child);
}
