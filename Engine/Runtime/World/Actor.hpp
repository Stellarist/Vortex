export module Runtime.World:Actor;

import Core;
import :Object;
import :Components.Component;
import :Components.SceneComponent;

export namespace Vortex {

class World;

class Actor : public Object {
private:
	std::vector<std::unique_ptr<Component>> components{};

	SceneComponent* root_component{};

	World* world{};

	bool enabled{true};
	bool tick_enabled{false};
	bool begun_play{false};

	void setWorld(World* world);

	void beginPlayInternal();
	void endPlayInternal();

	friend class World;

public:
	Actor(std::string name = "Actor");
	~Actor() noexcept override;

	Actor(const Actor&) = delete;
	Actor& operator=(const Actor&) = delete;

	Actor(Actor&&) noexcept = delete;
	Actor& operator=(Actor&&) noexcept = delete;

	World* getWorld() const noexcept;

	SceneComponent* getRootComponent() noexcept;
	const SceneComponent* getRootComponent() const noexcept;
	bool hasRootComponent() const noexcept;

	Transform* getTransform() noexcept;
	const Transform* getTransform() const noexcept;

	Actor* getParent() const noexcept;
	auto getAttachedActors() const -> std::vector<Actor*>;

	void attachActor(Actor& actor);
	void detachFromParent();

	template <IsComponent T>
	T& addComponent(std::unique_ptr<T> component);

	template <IsComponent T, typename... Args>
	T& addComponent(Args&&... args);

	Component& addComponent(std::unique_ptr<Component> component);
	bool removeComponent(Component& component);

	template <IsComponent T>
	T& getComponent() const;

	template <IsComponent T>
	auto getComponents() const -> std::vector<T*>;

	template <IsComponent T>
	bool hasComponent() const noexcept;

	auto getOwnedComponents() const noexcept -> const std::vector<std::unique_ptr<Component>>&;

	bool isEnabled() const noexcept;
	auto setEnabled(bool enabled) noexcept -> Actor&;

	bool isTickEnabled() const noexcept;
	auto setTickEnabled(bool enabled) noexcept -> Actor&;

	bool hasBegunPlay() const noexcept;

	virtual void beginPlay();
	virtual void tick(float dt);
	virtual void endPlay();
};

template <IsComponent T>
T& Actor::addComponent(std::unique_ptr<T> component)
{
	CHECK(Argument, component, "Cannot add an empty component to an actor");

	auto* result = component.get();
	addComponent(std::unique_ptr<Component>(std::move(component)));
	return *result;
}

template <IsComponent T, typename... Args>
T& Actor::addComponent(Args&&... args)
{
	return addComponent(std::make_unique<T>(std::forward<Args>(args)...));
}

template <IsComponent T>
T& Actor::getComponent() const
{
	for (const auto& component : components)
		if (auto* result = dynamic_cast<T*>(component.get()))
			return *result;

	ERROR(Range, "Actor does not contain the requested component type");
}

template <IsComponent T>
auto Actor::getComponents() const -> std::vector<T*>
{
	std::vector<T*> result;
	for (const auto& component : components)
		if (auto* typed_component = dynamic_cast<T*>(component.get()))
			result.push_back(typed_component);
	return result;
}

template <IsComponent T>
bool Actor::hasComponent() const noexcept
{
	return std::ranges::any_of(components, [](const auto& component) {
		return dynamic_cast<const T*>(component.get()) != nullptr;
	});
}

}        // namespace Vortex
