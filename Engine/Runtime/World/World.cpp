module;

#include <algorithm>

module Runtime.World;

namespace Vortex {

class ScopedFlag {
private:
	bool& value;
	bool previous;

public:
	ScopedFlag(bool& flag) : value(flag), previous(flag) { value = true; }
	~ScopedFlag() { value = previous; }
};


World::World(std::string world_name) :
    Object(std::move(world_name)) {}

World::~World()
{
	clearActors();
}

void World::clearActors()
{
	updating = false;
	endPlay();

	for (auto& actor : actors)
		actor->detachFromParent();

	for (auto& actor : actors) {
		actor->endPlayInternal();
		actor->setWorld(nullptr);
	}

	active_camera = nullptr;
	components.clear();
	actors.clear();
}

World& World::setActors(std::vector<std::unique_ptr<Actor>> new_actors)
{
	CHECK(!updating && !dispatching_lifecycle, "Actors cannot be replaced during tick or lifecycle callbacks");
	CHECK(Argument, !std::ranges::any_of(new_actors, [](const auto& actor) { return actor == nullptr; }),
	    "A world cannot own an empty actor");

	std::unordered_set<const Actor*> actor_set;
	for (const auto& actor : new_actors) {
		CHECK(actor->getWorld() == nullptr, "Cannot add an actor that already belongs to a world");
		actor_set.insert(actor.get());
	}

	for (const auto& actor : new_actors) {
		if (auto* parent = actor->getParent(); parent)
			CHECK(actor_set.contains(parent), "An attached actor graph must enter a world together");
		for (auto* attached_actor : actor->getAttachedActors())
			CHECK(actor_set.contains(attached_actor), "An attached actor graph must enter a world together");
	}

	const bool resume_play = playing;
	clearActors();

	actors = std::move(new_actors);

	{
		ScopedFlag registering(registering_actor);
		for (auto& actor : actors)
			actor->setWorld(this);
	}

	LOG(Debug, "World '{}' replaced its actor set ({} actors)", getName(), actors.size());
	if (resume_play)
		beginPlay();

	return *this;
}

Actor& World::addActor(std::unique_ptr<Actor> actor)
{
	CHECK(!updating && !dispatching_lifecycle, "Actors cannot be added during tick or lifecycle callbacks");
	CHECK(Argument, actor, "Cannot add an empty actor to a world");
	CHECK(actor->getWorld() == nullptr, "Cannot add an actor that already belongs to a world");
	CHECK(!actor->getParent() && actor->getAttachedActors().empty(),
	    "An attached actor graph must be added together with setActors");

	auto* result = actor.get();

	{
		ScopedFlag registering(registering_actor);
		actor->setWorld(this);
	}

	actors.push_back(std::move(actor));
	if (playing) {
		ScopedFlag dispatching(dispatching_lifecycle);
		result->beginPlayInternal();
		for (const auto& component : result->getOwnedComponents())
			component->beginPlayInternal();
	}

	LOG(Debug, "World '{}' added actor '{}' ({} actors)",
	    getName(), result->getName(), actors.size());

	return *result;
}

bool World::removeActor(Actor& actor)
{
	if (actor.getWorld() != this)
		return false;

	CHECK(!updating && !dispatching_lifecycle, "Actors cannot be removed during tick or lifecycle callbacks");
	return removeActorImmediate(actor);
}

bool World::removeActorImmediate(Actor& actor)
{
	if (actor.getWorld() != this)
		return false;

	const auto actor_name = actor.getName();

	{
		ScopedFlag dispatching(dispatching_lifecycle);
		for (auto attached_actors = actor.getAttachedActors(); auto* attached_actor : attached_actors)
			attached_actor->detachFromParent();

		actor.detachFromParent();
		actor.endPlayInternal();
		actor.setWorld(nullptr);
	}

	auto it = std::ranges::find_if(actors, [&actor](const auto& owned_actor) {
		return owned_actor.get() == &actor;
	});

	if (it == actors.end())
		return false;

	actors.erase(it);
	LOG(Debug, "World '{}' removed actor '{}' ({} actors)", getName(), actor_name, actors.size());

	return true;
}

const std::vector<std::unique_ptr<Actor>>& World::getActors() const noexcept
{
	return actors;
}

std::vector<Actor*> World::getRootActors()
{
	std::vector<Actor*> result;
	for (const auto& actor : actors)
		if (!actor->getParent())
			result.push_back(actor.get());
	return result;
}

std::vector<const Actor*> World::getRootActors() const
{
	std::vector<const Actor*> result;
	for (const auto& actor : actors)
		if (!actor->getParent())
			result.push_back(actor.get());
	return result;
}

void World::registerComponent(Component& component)
{
	CHECK(component.getOwner() && component.getOwner()->getWorld() == this,
	    "Only components owned by an actor in this world can be registered");

	if (std::ranges::find(components, &component) != components.end())
		return;

	components.push_back(&component);

	{
		ScopedFlag dispatching(dispatching_lifecycle);
		component.registerComponent();
	}

	if (!active_camera) {
		auto* camera = dynamic_cast<CameraComponent*>(&component);
		if (camera && camera->isEnabled() && camera->getOwner() && camera->getOwner()->isEnabled())
			active_camera = camera;
	}

	if (playing && !registering_actor) {
		ScopedFlag dispatching(dispatching_lifecycle);
		component.beginPlayInternal();
	}
}

void World::unregisterComponent(Component& component)
{
	components.erase(std::remove(components.begin(), components.end(), &component), components.end());
	if (active_camera == &component) {
		active_camera = nullptr;
		refreshActiveCamera();
	}

	ScopedFlag dispatching(dispatching_lifecycle);
	component.unregisterComponent();
}

Actor* World::findActor(const std::string& actor_name)
{
	return const_cast<Actor*>(std::as_const(*this).findActor(actor_name));
}

const Actor* World::findActor(const std::string& actor_name) const
{
	for (const auto& actor : actors)
		if (actor->getName() == actor_name)
			return actor.get();
	return nullptr;
}

CameraComponent* World::getActiveCamera() const noexcept
{
	refreshActiveCamera();
	return active_camera;
}

World& World::setActiveCamera(CameraComponent* camera)
{
	CHECK(Argument, !camera || camera->getWorld() == this, "The active camera must belong to this world");
	CHECK(Argument, !camera || camera->isEnabled() && camera->getOwner() && camera->getOwner()->isEnabled(),
	    "The active camera and its owner must be enabled");

	if (active_camera == camera)
		return *this;

	active_camera = camera;
	LOG(Debug, "World '{}' active camera set to '{}'", getName(),
	    camera ? std::string_view(camera->getName()) : std::string_view("<automatic>"));

	return *this;
}

void World::refreshActiveCamera() const noexcept
{
	auto usable = [this](const CameraComponent* camera) {
		return camera && camera->getWorld() == this && camera->isEnabled() &&
		    camera->getOwner() && camera->getOwner()->isEnabled();
	};

	if (usable(active_camera))
		return;

	active_camera = nullptr;
	for (auto* camera : getComponents<CameraComponent>())
		if (usable(camera)) {
			active_camera = camera;
			break;
		}
}

bool World::isPlaying() const noexcept
{
	return playing;
}

bool World::isUpdating() const noexcept
{
	return updating;
}

void World::beginPlay()
{
	if (playing)
		return;

	LOG("World '{}' begin play ({} actors, {} components)", getName(), actors.size(), components.size());
	playing = true;

	{
		ScopedFlag dispatching(dispatching_lifecycle);
		for (const auto& actor : actors)
			actor->beginPlayInternal();
		for (auto* component : components)
			component->beginPlayInternal();
	}
}

void World::tick(float dt)
{
	if (!playing || updating || dispatching_lifecycle)
		return;

	{
		ScopedFlag ticking(updating);
		for (const auto& actor : actors)
			if (actor->begun_play && actor->enabled && actor->tick_enabled)
				actor->tick(dt);

		for (auto* component : components) {
			auto* owner = component->getOwner();
			if (!owner || !owner->isEnabled())
				continue;
			if (component->isRegistered() && component->hasBegunPlay() && component->isEnabled() && component->isTickEnabled())
				component->tickComponent(dt);
		}
	}
}

void World::endPlay()
{
	if (!playing)
		return;
	CHECK(!updating, "A world cannot end play while it is ticking");

	{
		ScopedFlag dispatching(dispatching_lifecycle);
		for (auto it = components.rbegin(); it != components.rend(); ++it)
			(*it)->endPlayInternal();
		for (auto it = actors.rbegin(); it != actors.rend(); ++it)
			(*it)->endPlayInternal();
	}

	playing = false;
	LOG("World '{}' end play", getName());
}

}        // namespace Vortex
