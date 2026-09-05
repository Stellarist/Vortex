export module Runtime.World:World;

import Core;
import :Object;
import :Actor;
import :Components.Component;
import :Components.CameraComponent;

export namespace Vortex {

class World : public Object {
private:
	std::vector<std::unique_ptr<Actor>> actors{};
	std::vector<Component*> components{};

	mutable CameraComponent* active_camera{};

	bool playing{false};
	bool updating{false};
	bool dispatching_lifecycle{false};
	bool registering_actor{false};

	void registerComponent(Component& component);
	void unregisterComponent(Component& component);

	bool removeActorImmediate(Actor& actor);
	void clearActors();
	void refreshActiveCamera() const noexcept;

	friend class Actor;

public:
	World() = default;
	World(std::string name);
	~World() override;

	World(const World&) = delete;
	World& operator=(const World&) = delete;

	World(World&&) noexcept = delete;
	World& operator=(World&&) noexcept = delete;

	auto setActors(std::vector<std::unique_ptr<Actor>> actors) -> World&;
	Actor& addActor(std::unique_ptr<Actor> actor);

	bool removeActor(Actor& actor);
	auto getActors() const noexcept -> const std::vector<std::unique_ptr<Actor>>&;

	auto getRootActors() -> std::vector<Actor*>;
	auto getRootActors() const -> std::vector<const Actor*>;

	template <IsComponent T>
	auto getComponents() const -> std::vector<T*>;

	template <IsComponent T>
	void clearComponents();

	template <IsComponent T>
	bool hasComponent() const noexcept;

	Actor* findActor(const std::string& name);
	const Actor* findActor(const std::string& name) const;

	CameraComponent* getActiveCamera() const noexcept;
	auto setActiveCamera(CameraComponent* camera) -> World&;

	bool isPlaying() const noexcept;
	bool isUpdating() const noexcept;

	void beginPlay();
	void tick(float dt);
	void endPlay();
};

template <IsComponent T>
auto World::getComponents() const -> std::vector<T*>
{
	std::vector<T*> result;
	result.reserve(components.size());
	for (auto* component : components)
		if (auto* typed_component = dynamic_cast<T*>(component))
			result.push_back(typed_component);
	return result;
}

template <IsComponent T>
void World::clearComponents()
{
	auto matching_components = getComponents<T>();
	for (auto* component : matching_components)
		if (auto* owner = component->getOwner())
			owner->removeComponent(*component);
}

template <IsComponent T>
bool World::hasComponent() const noexcept
{
	return std::ranges::any_of(components, [](const auto* component) {
		return dynamic_cast<const T*>(component) != nullptr;
	});
}

}        // namespace Vortex
