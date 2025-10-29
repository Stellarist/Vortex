#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Scene/Core/Component.hpp"

class Light : public Component {
private:
	glm::vec3 color{1.0f, 1.0f, 1.0f};
	float     intensity{1.0f};

public:
	Light(const std::string& name);

	Light(const Light&) = delete;
	Light& operator=(const Light&) = delete;

	Light(Light&& other) noexcept = default;
	Light& operator=(Light&& other) noexcept = default;

	~Light() override = default;

	std::type_index getType() override;

	glm::vec3 getColor() const;
	void      setColor(const glm::vec3& color);

	float getIntensity() const;
	void  setIntensity(float intensity);
};

class DirectionalLight : public Light {
private:
	glm::vec3 direction{0.0f, 0.0f, -1.0f};

public:
	DirectionalLight(const std::string& name);

	DirectionalLight(const DirectionalLight&) = delete;
	DirectionalLight& operator=(const DirectionalLight&) = delete;

	DirectionalLight(DirectionalLight&& other) = default;
	DirectionalLight& operator=(DirectionalLight&& other) = default;

	~DirectionalLight() override = default;

	std::type_index getType() override;

	glm::vec3 getDirection() const;
	void      setDirection(const glm::vec3& direction);
};

class PointLight : public Light {
private:
	float range{10.0f};

public:
	PointLight(const std::string& name);

	PointLight(const PointLight&) = delete;
	PointLight& operator=(const PointLight&) = delete;

	PointLight(PointLight&& other) noexcept = default;
	PointLight& operator=(PointLight&& other) noexcept = default;

	~PointLight() override = default;

	std::type_index getType() override;

	float getRange() const;
	void  setRange(float range);
};

class SpotLight : public Light {
private:
	glm::vec3 direction{0.0f, 0.0f, -1.0f};
	float     range{10.0f};
	float     inner_cone_angle{0.0f};
	float     outer_cone_angle{0.0f};

public:
	SpotLight(const std::string& name);
	~SpotLight() override = default;

	SpotLight(const SpotLight&) = delete;
	SpotLight& operator=(const SpotLight&) = delete;

	SpotLight(SpotLight&& other) noexcept = default;
	SpotLight& operator=(SpotLight&& other) noexcept = default;

	std::type_index getType() override;

	glm::vec3 getDirection() const;
	void      setDirection(const glm::vec3& direction);

	float getRange() const;
	void  setRange(float range);

	float getInnerConeAngle() const;
	void  setInnerConeAngle(float angle);

	float getOuterConeAngle() const;
	void  setOuterConeAngle(float angle);
};
