#pragma once

#include <glm/glm.hpp>

class Ray {
private:
	glm::vec3 origin_vec;
	glm::vec3 direction_vec;
	glm::vec3 inv_direction_vec;

public:
	Ray(const glm::vec3& origin, const glm::vec3& direction);

	const glm::vec3& origin() const;
	const glm::vec3& direction() const;
	const glm::vec3& invDirection() const;
};
