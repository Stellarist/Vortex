#pragma once

#include <memory>
#include <string>
#include <vector>
#include <typeindex>
#include <algorithm>

#include "Node.hpp"
#include "Component.hpp"
#include "Resource.hpp"

template <typename T>
concept IsResource = std::is_base_of<Resource, T>::value;

class Scene : public Entity {
private:
	std::string name;

	Node*  root{};
	World* world{};

	std::vector<std::unique_ptr<Node>> nodes;

	std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> components;
	std::unordered_map<std::type_index, std::vector<std::shared_ptr<Resource>>>  resources;

	std::vector<std::unique_ptr<Behaviour>> behaviours;
	std::vector<Behaviour*>                 tickable_behaviours;

public:
	Scene() = default;
	Scene(std::string name);
	Scene(const std::string& name);
	~Scene() override = default;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Scene(Scene&&) noexcept = default;
	Scene& operator=(Scene&&) noexcept = default;

	std::type_index getType() override;

	auto getName() const -> const std::string&;
	void setName(const std::string& name);

	void setNodes(std::vector<std::unique_ptr<Node>>&& nodes);
	void addNode(std::unique_ptr<Node>&& node);

	World* getWorld() const;
	void   setWorld(World& world);

	Node* getRoot();
	void  setRoot(Node& node);
	void  addChild(Node& child);

	template <IsComponent T>
	auto getComponents() const -> std::vector<T*>;
	auto getComponents(const std::type_index& type) const -> const std::vector<std::unique_ptr<Component>>&;

	template <IsComponent T>
	void setComponents(std::vector<std::unique_ptr<T>>&& components);
	void setComponents(const std::type_index& type, std::vector<std::unique_ptr<Component>>&& components);

	template <IsComponent T>
	void addComponent(std::unique_ptr<T>&& component);
	template <IsComponent T>
	void addComponent(std::unique_ptr<T>&& component, Node& node);

	template <IsComponent T>
	void clearComponents();
	void removeComponent(Component& component);

	template <IsComponent T>
	bool hasComponent() const;
	bool hasComponent(const std::type_index& type) const;

	template <IsResource T>
	auto getResources() const -> std::vector<std::shared_ptr<T>>;
	auto getResources(const std::type_index& type) const -> const std::vector<std::shared_ptr<Resource>>&;

	template <IsResource T>
	void setResources(std::vector<std::shared_ptr<T>>&& resources);
	void setResources(const std::type_index& type, std::vector<std::shared_ptr<Resource>>&& resources);

	template <IsResource T>
	void addResource(std::shared_ptr<T> resource);

	template <IsResource T>
	void clearResources();
	void removeResource(Resource& resource);

	template <IsResource T>
	bool hasResource() const;
	bool hasResource(const std::type_index& type) const;

	template <IsBehaviour T>
	auto getBehaviour() const -> T*;
	auto getBehaviours() const -> const std::vector<std::unique_ptr<Behaviour>>&;

	void addBehaviour(std::unique_ptr<Behaviour>&& behaviour);
	void addBehaviour(std::unique_ptr<Behaviour>&& behaviour, Node& node);

	void removeBehaviour(Behaviour& behaviour);
	void refreshBehaviours();

	Node* findNode(const std::string& name);

	void start();
	void update(float dt);
};

template <IsComponent T>
auto Scene::getComponents() const -> std::vector<T*>
{
	std::vector<T*> result;
	if (hasComponent(typeid(T))) {
		auto& scene_components = getComponents(typeid(T));
		result.resize(scene_components.size());
		std::transform(
		    scene_components.begin(), scene_components.end(), result.begin(),
		    [](auto& component) { return dynamic_cast<T*>(component.get()); });
	}
	return result;
}

template <IsComponent T>
void Scene::setComponents(std::vector<std::unique_ptr<T>>&& components)
{
	std::vector<std::unique_ptr<Component>> result(components.size());
	std::move(components.begin(), components.end(), result.begin());
	setComponents(typeid(T), std::move(result));
}

template <IsComponent T>
void Scene::addComponent(std::unique_ptr<T>&& component)
{
	if (component)
		components[typeid(T)].push_back(std::move(component));
}

template <IsComponent T>
void Scene::addComponent(std::unique_ptr<T>&& component, Node& node)
{
	node.setComponent(*component);
	if (component)
		components[typeid(T)].push_back(std::move(component));
}

template <IsComponent T>
void Scene::clearComponents()
{
	setComponents<T>(typeid(T), {});
}

template <IsComponent T>
bool Scene::hasComponent() const
{
	return hasComponent(typeid(T));
}

template <IsResource T>
auto Scene::getResources() const -> std::vector<std::shared_ptr<T>>
{
	std::vector<std::shared_ptr<T>> result;
	if (hasResource(typeid(T))) {
		auto& scene_resources = getResources(typeid(T));
		result.resize(scene_resources.size());
		std::transform(scene_resources.begin(), scene_resources.end(),
		    result.begin(), [](const auto& resource) {
			    return std::dynamic_pointer_cast<T>(resource);
		    });
	}
	return result;
}

template <IsResource T>
void Scene::setResources(std::vector<std::shared_ptr<T>>&& resources)
{
	std::vector<std::shared_ptr<Resource>> result(
	    std::make_move_iterator(resources.begin()),
	    std::make_move_iterator(resources.end()));
	setResources(typeid(T), std::move(result));
}

template <IsResource T>
void Scene::addResource(std::shared_ptr<T> resource)
{
	if (resource)
		resources[typeid(T)].push_back(std::move(resource));
}

template <IsResource T>
void Scene::clearResources()
{
	resources.erase(typeid(T));
}

template <IsResource T>
bool Scene::hasResource() const
{
	return hasResource(typeid(T));
}

template <IsBehaviour T>
T* Scene::getBehaviour() const
{
	for (auto& behaviour : behaviours)
		if (auto* result = dynamic_cast<T*>(behaviour.get()))
			return result;

	return nullptr;
}
