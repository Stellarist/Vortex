module Runtime.World;

namespace Vortex {

static Vec3 forwardDirection(const SceneComponent& component) noexcept
{
	return Math::normalize(Vec3(component.getWorldMatrix() * Vec4(0.0f, -1.0f, 0.0f, 0.0f)));
}

LightComponent::LightComponent(std::string component_name) :
    SceneComponent(std::move(component_name))
{}

Vec3 LightComponent::getColor() const noexcept
{
	return color;
}

LightComponent& LightComponent::setColor(const Vec3& value) noexcept
{
	color = value;
	return *this;
}

float LightComponent::getIntensity() const noexcept
{
	return intensity;
}

LightComponent& LightComponent::setIntensity(float value) noexcept
{
	intensity = value;
	return *this;
}


DirectionalLightComponent::DirectionalLightComponent(std::string component_name) :
    LightComponent(std::move(component_name))
{}

Vec3 DirectionalLightComponent::getDirection() const noexcept
{
	return forwardDirection(*this);
}


PointLightComponent::PointLightComponent(std::string component_name) :
    LightComponent(std::move(component_name))
{}

float PointLightComponent::getRange() const noexcept
{
	return range;
}

PointLightComponent& PointLightComponent::setRange(float value) noexcept
{
	range = value;
	return *this;
}


SpotLightComponent::SpotLightComponent(std::string component_name) :
    LightComponent(std::move(component_name))
{}

Vec3 SpotLightComponent::getDirection() const noexcept
{
	return forwardDirection(*this);
}

float SpotLightComponent::getRange() const noexcept
{
	return range;
}

SpotLightComponent& SpotLightComponent::setRange(float value) noexcept
{
	range = value;
	return *this;
}

float SpotLightComponent::getInnerConeAngle() const noexcept
{
	return inner_cone_angle;
}

SpotLightComponent& SpotLightComponent::setInnerConeAngle(float value) noexcept
{
	inner_cone_angle = value;
	return *this;
}

float SpotLightComponent::getOuterConeAngle() const noexcept
{
	return outer_cone_angle;
}

SpotLightComponent& SpotLightComponent::setOuterConeAngle(float value) noexcept
{
	outer_cone_angle = value;
	return *this;
}

}        // namespace Vortex
