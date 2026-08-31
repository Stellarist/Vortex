module;

#include <algorithm>

module Runtime.World;

namespace Vortex {

SceneComponent::SceneComponent(std::string component_name) :
    Component(std::move(component_name))
{}

SceneComponent::~SceneComponent() noexcept
{
	detach();
	auto children = attach_children;
	for (auto* child : children)
		child->detach();
}

Transform& SceneComponent::getTransform() noexcept
{
	return transform;
}

const Transform& SceneComponent::getTransform() const noexcept
{
	return transform;
}

Mat4 SceneComponent::getWorldMatrix() const noexcept
{
	const Mat4 local_matrix = transform.getMatrix();
	return attach_parent ? attach_parent->getWorldMatrix() * local_matrix : local_matrix;
}

Vec3 SceneComponent::getWorldPosition() const noexcept
{
	return Vec3(getWorldMatrix()[3]);
}

SceneComponent* SceneComponent::getAttachParent() const noexcept
{
	return attach_parent;
}

const std::vector<SceneComponent*>& SceneComponent::getAttachChildren() const noexcept
{
	return attach_children;
}

SceneComponent& SceneComponent::attachTo(SceneComponent& parent)
{
	CHECK(Argument, &parent != this, "A scene component cannot attach to itself");

	for (auto* ancestor = &parent; ancestor; ancestor = ancestor->attach_parent)
		CHECK(ancestor != this, "Scene component attachment would create a cycle");

	CHECK(getOwner() && parent.getOwner(), "Scene components must have owners before attachment");

	CHECK(getOwner() != parent.getOwner() || getOwner()->getRootComponent() != this,
	    "An actor's root component cannot attach beneath another component of the same actor");

	CHECK(getOwner() == parent.getOwner() || getOwner()->getRootComponent() == this,
	    "Only an actor's root component can attach across actors");

	auto* world = getWorld();
	auto* parent_world = parent.getWorld();

	CHECK(world == parent_world || !world && !parent_world,
	    "Scene components from different worlds cannot be attached");

	if (attach_parent == &parent)
		return *this;

	detach();
	attach_parent = &parent;
	parent.attach_children.push_back(this);
	return *this;
}

SceneComponent& SceneComponent::detach()
{
	if (!attach_parent)
		return *this;

	auto& siblings = attach_parent->attach_children;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());

	attach_parent = nullptr;
	return *this;
}

}        // namespace Vortex
