module;

#include <algorithm>
module Runtime.World;

namespace Vortex {

class ScopedFlag {
private:
	bool& value;
	bool  previous;

public:
	ScopedFlag(bool& flag) : value(flag), previous(flag) { value = true; }
	~ScopedFlag() { value = previous; }
};


Scene::Scene(std::string scene_name) :
    Object(std::move(scene_name))
{}

Scene::~Scene()
{
	teardownActors();
}

void Scene::teardownActors()
{
	updating = false;
	endPlay();

	for (auto& actor : actors)
		actor->detachFromParent();
	for (auto& actor : actors) {
		actor->endPlayInternal();
		actor->setScene(nullptr);
	}

	components.clear();
	actors.clear();
}

Scene& Scene::setActors(std::vector<std::unique_ptr<Actor>> new_actors)
{
	if (updating || dispatching_lifecycle)
		throw std::logic_error("Actors cannot be replaced during tick or lifecycle callbacks");
	if (std::ranges::any_of(new_actors, [](const auto& actor) { return actor == nullptr; }))
		throw std::invalid_argument("A scene cannot own an empty actor");

	std::unordered_set<const Actor*> actor_set;
	for (const auto& actor : new_actors) {
		if (actor->getScene())
			throw std::logic_error("Cannot add an actor that already belongs to a scene");

		actor_set.insert(actor.get());
	}

	for (const auto& actor : new_actors) {
		if (auto* parent = actor->getParent(); parent && !actor_set.contains(parent))
			throw std::logic_error("An attached actor graph must enter a scene together");

		for (auto* attached_actor : actor->getAttachedActors())
			if (!actor_set.contains(attached_actor))
				throw std::logic_error("An attached actor graph must enter a scene together");
	}

	const bool resume_play = playing;
	teardownActors();
	actors = std::move(new_actors);

	{
		ScopedFlag registering(registering_actor);
		for (auto& actor : actors)
			actor->setScene(this);
	}

	if (resume_play)
		beginPlay();
	return *this;
}

Actor& Scene::addActor(std::unique_ptr<Actor> actor)
{
	if (updating || dispatching_lifecycle)
		throw std::logic_error("Actors cannot be added during tick or lifecycle callbacks");
	if (!actor)
		throw std::invalid_argument("Cannot add an empty actor to a scene");
	if (actor->getScene())
		throw std::logic_error("Cannot add an actor that already belongs to a scene");
	if (actor->getParent() || !actor->getAttachedActors().empty())
		throw std::logic_error("An attached actor graph must be added together with setActors");

	auto* result = actor.get();
	{
		ScopedFlag registering(registering_actor);
		actor->setScene(this);
	}
	actors.push_back(std::move(actor));

	if (playing) {
		ScopedFlag dispatching(dispatching_lifecycle);
		result->beginPlayInternal();
		for (const auto& component : result->getOwnedComponents())
			component->beginPlayInternal();
	}
	return *result;
}

bool Scene::removeActor(Actor& actor)
{
	if (actor.getScene() != this)
		return false;
	if (updating || dispatching_lifecycle)
		throw std::logic_error("Actors cannot be removed during tick or lifecycle callbacks");
	return removeActorImmediate(actor);
}

bool Scene::removeActorImmediate(Actor& actor)
{
	if (actor.getScene() != this)
		return false;

	{
		ScopedFlag dispatching(dispatching_lifecycle);
		auto       attached_actors = actor.getAttachedActors();
		for (auto* attached_actor : attached_actors) {
			auto*      attached_root = attached_actor->getRootComponent();
			const Mat4 world_matrix = attached_root->getWorldMatrix();
			attached_actor->detachFromParent();
			attached_root->getTransform().setMatrix(world_matrix);
		}

		actor.detachFromParent();
		actor.endPlayInternal();
		actor.setScene(nullptr);
	}

	auto it = std::ranges::find_if(actors, [&actor](const auto& owned_actor) {
		return owned_actor.get() == &actor;
	});
	if (it == actors.end())
		return false;
	actors.erase(it);
	return true;
}

const std::vector<std::unique_ptr<Actor>>& Scene::getActors() const noexcept
{
	return actors;
}

std::vector<Actor*> Scene::getRootActors()
{
	std::vector<Actor*> result;
	for (const auto& actor : actors)
		if (!actor->getParent())
			result.push_back(actor.get());
	return result;
}

std::vector<const Actor*> Scene::getRootActors() const
{
	std::vector<const Actor*> result;
	for (const auto& actor : actors)
		if (!actor->getParent())
			result.push_back(actor.get());
	return result;
}

World* Scene::getWorld() const noexcept
{
	return world;
}

void Scene::registerComponent(Component& component)
{
	if (!component.getOwner() || component.getOwner()->getScene() != this)
		throw std::logic_error("Only components owned by an actor in this scene can be registered");
	if (std::ranges::find(components, &component) != components.end())
		return;

	components.push_back(&component);
	{
		ScopedFlag dispatching(dispatching_lifecycle);
		component.registerComponent();
	}
	if (world)
		world->onComponentRegistered(component);

	if (playing && !registering_actor) {
		ScopedFlag dispatching(dispatching_lifecycle);
		component.beginPlayInternal();
	}
}

void Scene::unregisterComponent(Component& component)
{
	components.erase(std::remove(components.begin(), components.end(), &component), components.end());
	if (world)
		world->onComponentUnregistering(component);
	ScopedFlag dispatching(dispatching_lifecycle);
	component.unregisterComponent();
}

Actor* Scene::findActor(const std::string& actor_name)
{
	return const_cast<Actor*>(std::as_const(*this).findActor(actor_name));
}

const Actor* Scene::findActor(const std::string& actor_name) const
{
	for (const auto& actor : actors)
		if (actor->getName() == actor_name)
			return actor.get();
	return nullptr;
}

bool Scene::isPlaying() const noexcept
{
	return playing;
}

bool Scene::isUpdating() const noexcept
{
	return updating;
}

void Scene::beginPlay()
{
	if (playing)
		return;
	playing = true;
	{
		ScopedFlag dispatching(dispatching_lifecycle);
		for (const auto& actor : actors)
			actor->beginPlayInternal();
		for (auto* component : components)
			component->beginPlayInternal();
	}
}

void Scene::tick(float dt)
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

void Scene::endPlay()
{
	if (!playing)
		return;
	if (updating)
		throw std::logic_error("A scene cannot end play while it is ticking");

	{
		ScopedFlag dispatching(dispatching_lifecycle);
		for (auto it = components.rbegin(); it != components.rend(); ++it)
			(*it)->endPlayInternal();

		for (auto it = actors.rbegin(); it != actors.rend(); ++it)
			(*it)->endPlayInternal();
	}
	playing = false;
}

}        // namespace Vortex
