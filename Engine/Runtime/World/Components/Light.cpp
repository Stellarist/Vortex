module Runtime.World;

namespace Vortex {

Light::Light(const std::string& name) :
    Component(name)
{}

std::type_index Light::getType()
{
	return typeid(Light);
}

Vec3 Light::getColor() const
{
	return color;
}

void Light::setColor(const Vec3& color)
{
	this->color = color;
}

float Light::getIntensity() const
{
	return intensity;
}

void Light::setIntensity(float intensity)
{
	this->intensity = intensity;
}

DirectionalLight::DirectionalLight(const std::string& name) :
    Light{name}
{}

std::type_index DirectionalLight::getType()
{
	return typeid(DirectionalLight);
}

Vec3 DirectionalLight::getDirection() const
{
	return direction;
}

void DirectionalLight::setDirection(const Vec3& direction)
{
	this->direction = direction;
}

PointLight::PointLight(const std::string& name) :
    Light{name}
{}

std::type_index PointLight::getType()
{
	return typeid(PointLight);
}

float PointLight::getRange() const
{
	return range;
}

void PointLight::setRange(float range)
{
	this->range = range;
}

SpotLight::SpotLight(const std::string& name) :
    Light{name}
{}

std::type_index SpotLight::getType()
{
	return typeid(SpotLight);
}

Vec3 SpotLight::getDirection() const
{
	return direction;
}

void SpotLight::setDirection(const Vec3& direction)
{
	this->direction = direction;
}

float SpotLight::getRange() const
{
	return range;
}

void SpotLight::setRange(float range)
{
	this->range = range;
}

float SpotLight::getInnerConeAngle() const
{
	return inner_cone_angle;
}

void SpotLight::setInnerConeAngle(float angle)
{
	this->inner_cone_angle = angle;
}

float SpotLight::getOuterConeAngle() const
{
	return outer_cone_angle;
}

void SpotLight::setOuterConeAngle(float angle)
{
	this->outer_cone_angle = angle;
}

}        // namespace Vortex
