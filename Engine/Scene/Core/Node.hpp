#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Component.hpp"
#include "Behaviour.hpp"
#include "Scene/Components/Transform.hpp"

class Scene;
class World;

template <typename T>
concept IsComponent = std::is_base_of<Component, T>::value;

template <typename T>
concept IsBehaviour = std::is_base_of<Behaviour, T>::value;

class Node : public Entity {
private:
	size_t      id;
	std::string name;
	Node*       parent{};
	Transform   transform;

	Scene* scene{};
	World* world{};

	std::vector<Node*> children;

	std::unordered_map<std::type_index, Component*> components;
	std::vector<Behaviour*>                         behaviours;

public:
	Node(size_t id, std::string name);
	~Node() override = default;

	Node(const Node&) = delete;
	Node& operator=(Node&) = delete;

	Node(Node&&) noexcept = delete;
	Node& operator=(Node&&) noexcept = delete;

	std::type_index getType() override;

	size_t getId() const;

	const std::string& getName() const;
	void               setName(const std::string& name);

	Node* getParent() const;
	void  setParent(Node& parent);

	Transform& getTransform();

	Scene* getScene() const;
	void   setScene(Scene& scene);

	World* getWorld() const;

	template <IsComponent T>
	T&         getComponent() const;
	Component& getComponent(std::type_index type) const;
	void       setComponent(Component& component);

	template <IsComponent T>
	bool hasComponent() const;
	bool hasComponent(std::type_index type) const;

	template <IsBehaviour T>
	T*   getBehaviour() const;
	void addBehaviour(Behaviour& behaviour);
	void removeBehaviour(Behaviour& behaviour);

	auto getBehaviours() const -> const std::vector<Behaviour*>&;

	const std::vector<Node*>& getChildren() const;
	void                      addChild(Node& child);
};

template <IsComponent T>
T& Node::getComponent() const
{
	return dynamic_cast<T&>(getComponent(typeid(T)));
}

template <IsComponent T>
inline bool Node::hasComponent() const
{
	return hasComponent(typeid(T));
}

template <IsBehaviour T>
T* Node::getBehaviour() const
{
	for (auto* behaviour : behaviours)
		if (dynamic_cast<T*>(behaviour))
			return dynamic_cast<T&>(*behaviour);

	return nullptr;
}
