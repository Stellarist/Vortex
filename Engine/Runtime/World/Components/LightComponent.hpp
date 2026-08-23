export module Runtime.World:LightComponent;

import Core;
import :SceneComponent;

export namespace Vortex {

class LightComponent : public SceneComponent {
protected:
	Vec3  color{1.0f, 1.0f, 1.0f};
	float intensity{1.0f};

public:
	LightComponent(std::string name);
	~LightComponent() override = default;

	Vec3 getColor() const noexcept;
	auto setColor(const Vec3& color) noexcept -> LightComponent&;

	float getIntensity() const noexcept;
	auto  setIntensity(float intensity) noexcept -> LightComponent&;
};


class DirectionalLightComponent : public LightComponent {
public:
	DirectionalLightComponent(std::string name);
	~DirectionalLightComponent() override = default;

	Vec3 getDirection() const noexcept;
};


class PointLightComponent : public LightComponent {
private:
	float range{10.0f};

public:
	PointLightComponent(std::string name);
	~PointLightComponent() override = default;

	float getRange() const noexcept;
	auto  setRange(float range) noexcept -> PointLightComponent&;
};


class SpotLightComponent : public LightComponent {
private:
	float range{10.0f};
	float inner_cone_angle{0.0f};
	float outer_cone_angle{0.0f};

public:
	SpotLightComponent(std::string name);
	~SpotLightComponent() override = default;

	Vec3 getDirection() const noexcept;

	float getRange() const noexcept;
	auto  setRange(float range) noexcept -> SpotLightComponent&;

	float getInnerConeAngle() const noexcept;
	auto  setInnerConeAngle(float angle) noexcept -> SpotLightComponent&;

	float getOuterConeAngle() const noexcept;
	auto  setOuterConeAngle(float angle) noexcept -> SpotLightComponent&;
};

}        // namespace Vortex
