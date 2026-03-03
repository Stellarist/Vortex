#include "Ray.hpp"

Ray::Ray(const glm::vec3& origin, const glm::vec3& direction) :
    origin_vec(origin), direction_vec(glm::normalize(direction)), inv_direction_vec(1.0f / direction_vec)
{}

const glm::vec3& Ray::origin() const
{
	return origin_vec;
}

const glm::vec3& Ray::direction() const
{
	return direction_vec;
}

const glm::vec3& Ray::invDirection() const
{
	return inv_direction_vec;
}
