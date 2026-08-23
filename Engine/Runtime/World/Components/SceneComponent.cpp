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
	if (&parent == this)
		throw std::invalid_argument("A scene component cannot attach to itself");

	for (auto* ancestor = &parent; ancestor; ancestor = ancestor->attach_parent)
		if (ancestor == this)
			throw std::logic_error("Scene component attachment would create a cycle");

	if (!getOwner() || !parent.getOwner())
		throw std::logic_error("Scene components must have owners before attachment");

	if (getOwner() == parent.getOwner() && getOwner()->getRootComponent() == this)
		throw std::logic_error("An actor's root component cannot attach beneath another component of the same actor");

	if (getOwner() != parent.getOwner() && getOwner()->getRootComponent() != this)
		throw std::logic_error("Only an actor's root component can attach across actors");

	auto* scene = getScene();
	auto* parent_scene = parent.getScene();

	if (scene != parent_scene && (scene || parent_scene))
		throw std::logic_error("Scene components from different scenes cannot be attached");

	if (attach_parent == &parent)
		return *this;

	detach();
	attach_parent = &parent;
	parent.attach_children.push_back(this);
	return *this;
}

SceneComponent& SceneComponent::detach() noexcept
{
	if (!attach_parent)
		return *this;

	auto& siblings = attach_parent->attach_children;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());

	attach_parent = nullptr;
	return *this;
}

}        // namespace Vortex
