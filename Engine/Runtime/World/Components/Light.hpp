export module Runtime.World:Light;

import Core;
import :Component;

export namespace Vortex {

class Light : public Component {
protected:
	Vec3  color{1.0f, 1.0f, 1.0f};
	float intensity{1.0f};

public:
	Light(const std::string& name);
	~Light() override = default;

	std::type_index getType() override;

	Vec3 getColor() const;
	void setColor(const Vec3& color);

	float getIntensity() const;
	void  setIntensity(float intensity);
};

class DirectionalLight : public Light {
private:
	Vec3 direction{0.0f, 0.0f, -1.0f};

public:
	DirectionalLight(const std::string& name);
	~DirectionalLight() override = default;

	std::type_index getType() override;

	Vec3 getDirection() const;
	void setDirection(const Vec3& direction);
};

class PointLight : public Light {
private:
	float range{10.0f};

public:
	PointLight(const std::string& name);
	~PointLight() override = default;

	std::type_index getType() override;

	float getRange() const;
	void  setRange(float range);
};

class SpotLight : public Light {
private:
	Vec3  direction{0.0f, 0.0f, -1.0f};
	float range{10.0f};
	float inner_cone_angle{0.0f};
	float outer_cone_angle{0.0f};

public:
	SpotLight(const std::string& name);
	~SpotLight() override = default;

	std::type_index getType() override;

	Vec3 getDirection() const;
	void setDirection(const Vec3& direction);

	float getRange() const;
	void  setRange(float range);

	float getInnerConeAngle() const;
	void  setInnerConeAngle(float angle);

	float getOuterConeAngle() const;
	void  setOuterConeAngle(float angle);
};

}        // namespace Vortex
