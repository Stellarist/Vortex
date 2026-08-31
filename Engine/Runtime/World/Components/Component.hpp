export module Runtime.World:Components.Component;

import Core;
import :Object;

export namespace Vortex {

class Actor;
class World;

class Component : public Object {
private:
	Actor* owner{};

	bool enabled{true};
	bool tick_enabled{false};
	bool registered{false};
	bool begun_play{false};

	void setOwner(Actor* owner);

	void registerComponent();
	void unregisterComponent();

	void beginPlayInternal();
	void endPlayInternal();

	friend class Actor;
	friend class World;

public:
	Component() = default;
	Component(std::string name);
	~Component() noexcept override = default;

	Component(const Component&) = delete;
	Component& operator=(const Component&) = delete;

	Component(Component&&) noexcept = delete;
	Component& operator=(Component&&) noexcept = delete;

	Actor* getOwner() const noexcept;
	World* getWorld() const noexcept;

	bool isEnabled() const noexcept;
	auto setEnabled(bool enabled) noexcept -> Component&;

	bool isTickEnabled() const noexcept;
	auto setTickEnabled(bool enabled) noexcept -> Component&;

	bool isRegistered() const noexcept;
	bool hasBegunPlay() const noexcept;

	virtual void onRegister();
	virtual void beginPlay();
	virtual void tickComponent(float dt);
	virtual void endPlay();
	virtual void onUnregister();
};

template <typename T>
concept IsComponent = std::is_base_of_v<Component, T>;

}        // namespace Vortex
