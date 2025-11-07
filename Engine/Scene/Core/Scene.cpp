#include "Scene.hpp"

#include <queue>

Scene::Scene(std::string name) :
    name(std::move(name))
{}

Scene::Scene(const std::string& name) :
    root(new Node(name)),
    name(name)
{}

std::type_index Scene::getType()
{
	return typeid(Scene);
}

const std::string& Scene::getName() const
{
	return name;
}

void Scene::setName(const std::string& name)
{
	this->name = name;
}

void Scene::setNodes(std::vector<std::unique_ptr<Node>>&& nodes)
{
	assert(!nodes.empty() && "Nodes cannot be empty.");

	for (auto& node : nodes)
		node->setScene(*this);
	this->nodes = std::move(nodes);
}

void Scene::addNode(std::unique_ptr<Node>&& node)
{
	node->setScene(*this);
	nodes.push_back(std::move(node));
}

World* Scene::getWorld() const
{
	return world;
}

void Scene::setWorld(World& world)
{
	this->world = &world;
}

Node* Scene::getRoot()
{
	return root;
}

void Scene::setRoot(Node& node)
{
	this->root = &node;
}

void Scene::addChild(Node& child)
{
	root->addChild(child);
}

auto Scene::getComponents(const std::type_index& type) const -> const std::vector<std::unique_ptr<Component>>&
{
	return components.at(type);
}

void Scene::setComponents(const std::type_index& type, std::vector<std::unique_ptr<Component>>&& components)
{
	this->components[type] = std::move(components);
}

void Scene::removeComponent(Component& component)
{
	auto it = components.find(component.getType());
	if (it == components.end())
		return;

	auto& component_list = it->second;
	component_list.erase(std::remove_if(component_list.begin(), component_list.end(),
	                         [&component](const std::unique_ptr<Component>& c) {
		                         return c.get() == &component;
	                         }),
	    component_list.end());

	if (component_list.empty())
		components.erase(it);
}

bool Scene::hasComponent(const std::type_index& type) const
{
	return components.count(type) > 0;
}

auto Scene::getResources(const std::type_index& type) const -> const std::vector<std::shared_ptr<Resource>>&
{
	return resources.at(type);
}

void Scene::setResources(const std::type_index& type, std::vector<std::shared_ptr<Resource>>&& resources)
{
	this->resources[type] = std::move(resources);
}

void Scene::removeResource(Resource& resource)
{
	auto it = resources.find(resource.getType());
	if (it == resources.end())
		return;

	auto& resource_list = it->second;
	resource_list.erase(std::remove_if(resource_list.begin(), resource_list.end(),
	                        [&resource](const std::shared_ptr<Resource>& r) {
		                        return r.get() == &resource;
	                        }),
	    resource_list.end());

	if (resource_list.empty())
		resources.erase(it);
}

bool Scene::hasResource(const std::type_index& type) const
{
	return resources.count(type) > 0;
}

auto Scene::getBehaviours() const -> const std::vector<std::unique_ptr<Behaviour>>&
{
	return behaviours;
}

void Scene::addBehaviour(std::unique_ptr<Behaviour>&& behaviour)
{
	behaviours.push_back(std::move(behaviour));
	refreshBehaviours();
}

void Scene::addBehaviour(std::unique_ptr<Behaviour>&& behaviour, Node& node)
{
	node.addBehaviour(*behaviour);
	behaviours.push_back(std::move(behaviour));
	refreshBehaviours();
}

void Scene::removeBehaviour(Behaviour& behaviour)
{
	behaviours.erase(
	    std::remove_if(behaviours.begin(), behaviours.end(),
	        [&behaviour](const std::unique_ptr<Behaviour>& b) {
		        return b.get() == &behaviour;
	        }),
	    behaviours.end());
	refreshBehaviours();
}

void Scene::refreshBehaviours()
{
	tickable_behaviours.clear();
	for (auto& behaviour : behaviours)
		tickable_behaviours.push_back(behaviour.get());
}

Node* Scene::findNode(const std::string& name)
{
	for (auto* node : root->getChildren()) {
		std::queue<Node*> traverse_node;
		traverse_node.push(node);

		while (!traverse_node.empty()) {
			auto* node = traverse_node.front();
			traverse_node.pop();
			if (node->getName() == name)
				return node;

			for (auto* child_node : node->getChildren())
				traverse_node.push(child_node);
		}
	}
	return nullptr;
}

void Scene::start()
{
	for (auto& behaviour : behaviours)
		if (!behaviour->isStarted() && behaviour->isEnabled())
			behaviour->start();
}

void Scene::update(float dt)
{
	for (auto* behaviour : tickable_behaviours) {
		if (behaviour->isEnabled()) {
			if (!behaviour->isStarted())
				behaviour->start();

			behaviour->update(dt);
		}
	}
}
