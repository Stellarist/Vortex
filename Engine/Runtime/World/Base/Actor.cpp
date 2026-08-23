module Runtime.World;

import std;

namespace Vortex {

Actor::Actor(std::string actor_name) :
    Object(std::move(actor_name))
{}

Actor::~Actor() noexcept = default;

Scene* Actor::getScene() const noexcept
{
	return scene;
}

World* Actor::getWorld() const noexcept
{
	return scene ? scene->getWorld() : nullptr;
}

void Actor::setScene(Scene* new_scene)
{
	if (scene == new_scene)
		return;

	if (scene && new_scene)
		throw std::logic_error("An actor cannot belong to multiple scenes");

	if (scene) {
		auto* previous_scene = scene;
		for (size_t index = 0; index < components.size(); ++index)
			previous_scene->unregisterComponent(*components[index]);

		scene = nullptr;
		return;
	}

	scene = new_scene;
	if (scene)
		for (size_t index = 0; index < components.size(); ++index)
			scene->registerComponent(*components[index]);
}

SceneComponent* Actor::getRootComponent() noexcept
{
	return root_component;
}

const SceneComponent* Actor::getRootComponent() const noexcept
{
	return root_component;
}

bool Actor::hasRootComponent() const noexcept
{
	return root_component != nullptr;
}

Transform* Actor::getTransform() noexcept
{
	return root_component ? &root_component->getTransform() : nullptr;
}

const Transform* Actor::getTransform() const noexcept
{
	return root_component ? &root_component->getTransform() : nullptr;
}

Actor* Actor::getParent() const noexcept
{
	if (!root_component)
		return nullptr;

	auto* parent_component = root_component->getAttachParent();
	return parent_component ? parent_component->getOwner() : nullptr;
}

std::vector<Actor*> Actor::getAttachedActors() const
{
	std::vector<Actor*> result;
	if (!root_component)
		return result;

	auto collect_children = [this, &result](auto&& self, const SceneComponent& parent_component) -> void {
		for (auto* child_component : parent_component.getAttachChildren()) {
			auto* child_owner = child_component->getOwner();

			if (child_owner && child_owner != this && child_owner->getRootComponent() == child_component) {
				result.push_back(child_owner);
				continue;
			}

			if (child_owner == this)
				self(self, *child_component);
		}
	};

	collect_children(collect_children, *root_component);
	return result;
}

void Actor::attachActor(Actor& actor)
{
	if (&actor == this)
		throw std::invalid_argument("An actor cannot attach to itself");

	if (!root_component || !actor.root_component)
		throw std::logic_error("Both actors must have root scene components before attachment");

	if (scene != actor.scene && (scene || actor.scene))
		throw std::logic_error("Actors from different scenes cannot be attached");

	actor.root_component->attachTo(*root_component);
}

void Actor::detachFromParent() noexcept
{
	if (root_component)
		root_component->detach();
}

Component& Actor::addComponent(std::unique_ptr<Component> component)
{
	if (!component)
		throw std::invalid_argument("Cannot add an empty component to an actor");

	if (scene && (scene->updating || scene->dispatching_lifecycle))
		throw std::logic_error("Components cannot be added during tick or lifecycle callbacks");

	if (component->getOwner())
		throw std::logic_error("A component can only have one owning actor");

	auto* result = component.get();
	component->setOwner(this);
	components.push_back(std::move(component));

	if (auto* scene_component = dynamic_cast<SceneComponent*>(result)) {
		if (root_component)
			scene_component->attachTo(*root_component);
		else
			root_component = scene_component;
	}

	if (scene)
		scene->registerComponent(*result);

	return *result;
}

bool Actor::removeComponent(Component& component)
{
	if (&component == root_component || component.getOwner() != this)
		return false;

	if (scene && (scene->updating || scene->dispatching_lifecycle))
		throw std::logic_error("Components cannot be removed during tick or lifecycle callbacks");

	auto it = std::ranges::find_if(components, [&component](const auto& owned_component) {
		return owned_component.get() == &component;
	});

	if (it == components.end())
		return false;

	if (auto* scene_component = dynamic_cast<SceneComponent*>(&component)) {
		auto children = scene_component->getAttachChildren();
		for (auto* child : children)
			child->attachTo(*root_component);
		scene_component->detach();
	}

	if (scene)
		scene->unregisterComponent(component);
	component.setOwner(nullptr);

	it = std::ranges::find_if(components, [&component](const auto& owned_component) {
		return owned_component.get() == &component;
	});

	if (it != components.end())
		components.erase(it);

	return true;
}

const std::vector<std::unique_ptr<Component>>& Actor::getOwnedComponents() const noexcept
{
	return components;
}

bool Actor::isEnabled() const noexcept
{
	return enabled;
}

Actor& Actor::setEnabled(bool new_enabled) noexcept
{
	enabled = new_enabled;
	return *this;
}

bool Actor::isTickEnabled() const noexcept
{
	return tick_enabled;
}

Actor& Actor::setTickEnabled(bool new_enabled) noexcept
{
	tick_enabled = new_enabled;
	return *this;
}

bool Actor::hasBegunPlay() const noexcept
{
	return begun_play;
}

void Actor::beginPlayInternal()
{
	if (begun_play)
		return;

	beginPlay();
	begun_play = true;
}

void Actor::endPlayInternal()
{
	if (!begun_play)
		return;

	endPlay();
	begun_play = false;
}

void Actor::beginPlay()
{}

void Actor::tick(float)
{}

void Actor::endPlay()
{}

}        // namespace Vortex
