#include "Node.hpp"

#include "Scene.hpp"

Node::Node(std::string name) :
    name(std::move(name))
{
	transform.setNode(*this);
	setComponent<Transform>(transform);
}

std::type_index Node::getType()
{
	return typeid(Node);
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

Component& Node::getComponent(const std::type_index& type) const
{
	return *components.at(type);
}

void Node::setComponent(const std::type_index& type, Component& component)
{
	components[type] = &component;
	component.setNode(*this);
}

bool Node::hasComponent(const std::type_index& type) const
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
	child.parent = this;
	child.transform.invalidateWorldMatrix();
	children.push_back(&child);
}
