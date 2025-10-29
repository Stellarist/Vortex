#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Component.hpp"
#include "Behaviour.hpp"
#include "Transform.hpp"

class Scene;
class World;

class Node : public Entity {
private:
	size_t      id;
	std::string name;
	Node*       parent{};
	Transform   transform;

	Scene* scene;
	World* world{};

	std::vector<Node*> children;

	std::unordered_map<std::type_index, Component*> components;
	std::vector<Behaviour*>                         behaviours;

public:
	Node(size_t id, std::string name);

	Node(const Node&) = delete;
	Node& operator=(Node&) = delete;

	Node(Node&&) noexcept = delete;
	Node& operator=(Node&&) noexcept = delete;

	~Node() override = default;

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

	template <typename T>
	T&         getComponent() const;
	Component& getComponent(std::type_index type) const;
	void       setComponent(Component& component);

	template <typename T>
	bool hasComponent() const;
	bool hasComponent(std::type_index type) const;

	template <typename T>
	T*   getBehaviour() const;
	void addBehaviour(Behaviour& behaviour);
	void removeBehaviour(Behaviour& behaviour);

	auto getBehaviours() const -> const std::vector<Behaviour*>&;

	const std::vector<Node*>& getChildren() const;
	void                      addChild(Node& child);
};

template <typename T>
T& Node::getComponent() const
{
	static_assert(std::is_base_of<Component, T>::value, "T must be a Component type");

	return dynamic_cast<T&>(getComponent(typeid(T)));
}

template <typename T>
inline bool Node::hasComponent() const
{
	static_assert(std::is_base_of<Component, T>::value, "T must be a Component type");

	return hasComponent(typeid(T));
}

template <typename T>
T* Node::getBehaviour() const
{
	static_assert(std::is_base_of<Behaviour, T>::value, "T must be a Behaviour type");

	for (auto* behaviour : behaviours)
		if (dynamic_cast<T*>(behaviour))
			return dynamic_cast<T&>(*behaviour);

	return nullptr;
}
