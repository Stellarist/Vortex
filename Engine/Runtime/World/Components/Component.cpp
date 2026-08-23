module Runtime.World;

namespace Vortex {

Component::Component(std::string component_name) :
    Object(std::move(component_name))
{}

void Component::setOwner(Actor* new_owner)
{
	if (owner && new_owner && owner != new_owner)
		throw std::logic_error("A component cannot change its owning actor");

	owner = new_owner;
}

Actor* Component::getOwner() const noexcept
{
	return owner;
}

Scene* Component::getScene() const noexcept
{
	return owner ? owner->getScene() : nullptr;
}

World* Component::getWorld() const noexcept
{
	return owner ? owner->getWorld() : nullptr;
}

bool Component::isEnabled() const noexcept
{
	return enabled;
}

Component& Component::setEnabled(bool new_enabled) noexcept
{
	enabled = new_enabled;
	return *this;
}

bool Component::isTickEnabled() const noexcept
{
	return tick_enabled;
}

Component& Component::setTickEnabled(bool new_enabled) noexcept
{
	tick_enabled = new_enabled;
	return *this;
}

bool Component::isRegistered() const noexcept
{
	return registered;
}

bool Component::hasBegunPlay() const noexcept
{
	return begun_play;
}

void Component::registerComponent()
{
	if (registered)
		return;

	registered = true;
	onRegister();
}

void Component::unregisterComponent()
{
	if (!registered)
		return;

	endPlayInternal();
	onUnregister();
	registered = false;
}

void Component::beginPlayInternal()
{
	if (!registered || begun_play)
		return;

	beginPlay();
	begun_play = true;
}

void Component::endPlayInternal()
{
	if (!begun_play)
		return;

	endPlay();
	begun_play = false;
}

void Component::onRegister()
{}

void Component::beginPlay()
{}

void Component::tickComponent(float)
{}

void Component::endPlay()
{}

void Component::onUnregister()
{}

}        // namespace Vortex
