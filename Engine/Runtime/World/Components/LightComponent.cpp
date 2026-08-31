module Runtime.World;

namespace Vortex {

static float nonNegative(float value) noexcept
{
	return std::isfinite(value) ? std::max(value, 0.0f) : 0.0f;
}

static float coneAngle(float value) noexcept
{
	return std::clamp(std::isfinite(value) ? value : 0.0f, 0.0f, std::numbers::pi_v<float> * 0.5f);
}

static Vec3 forwardDirection(const SceneComponent& component) noexcept
{
	return Math::safeNormalize(Vec3(component.getWorldMatrix() * Vec4(0.0f, -1.0f, 0.0f, 0.0f)));
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
	color = Vec3{nonNegative(value.x), nonNegative(value.y), nonNegative(value.z)};
	return *this;
}

float LightComponent::getIntensity() const noexcept
{
	return intensity;
}

LightComponent& LightComponent::setIntensity(float value) noexcept
{
	intensity = nonNegative(value);
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
	range = nonNegative(value);
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
	range = nonNegative(value);
	return *this;
}

float SpotLightComponent::getInnerConeAngle() const noexcept
{
	return inner_cone_angle;
}

SpotLightComponent& SpotLightComponent::setInnerConeAngle(float value) noexcept
{
	inner_cone_angle = coneAngle(value);
	outer_cone_angle = std::max(outer_cone_angle, inner_cone_angle);
	return *this;
}

float SpotLightComponent::getOuterConeAngle() const noexcept
{
	return outer_cone_angle;
}

SpotLightComponent& SpotLightComponent::setOuterConeAngle(float value) noexcept
{
	outer_cone_angle = coneAngle(value);
	inner_cone_angle = std::min(inner_cone_angle, outer_cone_angle);
	return *this;
}

}        // namespace Vortex
