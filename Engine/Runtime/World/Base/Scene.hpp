export module Runtime.World:Scene;

import Core;
import Runtime.Object;
import :Actor;
import :Component;

export namespace Vortex {

class World;

class Scene : public Object {
private:
	World* world{};

	std::vector<std::unique_ptr<Actor>> actors;
	std::vector<Component*>             components;

	bool playing{false};
	bool updating{false};
	bool dispatching_lifecycle{false};
	bool registering_actor{false};

	void registerComponent(Component& component);
	void unregisterComponent(Component& component);

	bool removeActorImmediate(Actor& actor);
	void teardownActors();

	friend class Actor;
	friend class World;

public:
	Scene() = default;
	Scene(std::string name);
	~Scene() override;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) noexcept = delete;
	Scene& operator=(Scene&&) noexcept = delete;

	auto   setActors(std::vector<std::unique_ptr<Actor>> actors) -> Scene&;
	Actor& addActor(std::unique_ptr<Actor> actor);
	bool   removeActor(Actor& actor);
	auto   getActors() const noexcept -> const std::vector<std::unique_ptr<Actor>>&;
	auto   getRootActors() -> std::vector<Actor*>;
	auto   getRootActors() const -> std::vector<const Actor*>;

	World* getWorld() const noexcept;

	template <IsComponent T>
	auto getComponents() const -> std::vector<T*>;

	template <IsComponent T>
	void clearComponents();

	template <IsComponent T>
	bool hasComponent() const noexcept;

	Actor*       findActor(const std::string& name);
	const Actor* findActor(const std::string& name) const;

	bool isPlaying() const noexcept;
	bool isUpdating() const noexcept;
	void beginPlay();
	void tick(float dt);
	void endPlay();
};

template <IsComponent T>
auto Scene::getComponents() const -> std::vector<T*>
{
	std::vector<T*> result;
	result.reserve(components.size());
	for (auto* component : components)
		if (auto* typed_component = dynamic_cast<T*>(component))
			result.push_back(typed_component);
	return result;
}

template <IsComponent T>
void Scene::clearComponents()
{
	auto matching_components = getComponents<T>();
	for (auto* component : matching_components)
		if (auto* owner = component->getOwner())
			owner->removeComponent(*component);
}

template <IsComponent T>
bool Scene::hasComponent() const noexcept
{
	return std::ranges::any_of(components, [](const auto* component) {
		return dynamic_cast<const T*>(component) != nullptr;
	});
}

}        // namespace Vortex
