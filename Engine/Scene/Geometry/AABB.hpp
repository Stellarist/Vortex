#pragma once

#include <span>

#include <glm/glm.hpp>

#include "Ray.hpp"

struct AABB {
private:
	glm::vec3 min_bound;
	glm::vec3 max_bound;

public:
	AABB();
	AABB(const glm::vec3& min, const glm::vec3& max);

	glm::vec3 min() const;
	glm::vec3 max() const;

	glm::vec3 center() const;
	glm::vec3 scale() const;

	float area() const;
	float volume() const;

	void expand(const glm::vec3& point);
	void expand(const AABB& other);
	void expand(std::span<const glm::vec3> points);

	bool intersects(const AABB& other) const;
	bool intersects(const Ray& ray, float& tmin, float& tmax) const;

	bool contains(const glm::vec3& point) const;
	bool contains(const AABB& other) const;

	void reset();
	void transform(const glm::mat4& matrix);

	bool valid() const;
};
