module Runtime.World;

namespace Vortex {

PrimitiveComponent::PrimitiveComponent(std::string component_name) :
    SceneComponent(std::move(component_name))
{}

bool PrimitiveComponent::isVisible() const noexcept
{
	return visible;
}

PrimitiveComponent& PrimitiveComponent::setVisible(bool new_visible) noexcept
{
	visible = new_visible;
	return *this;
}

}        // namespace Vortex
