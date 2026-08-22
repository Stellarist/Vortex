module Runtime.World;

namespace Vortex {

Ray::Ray(const Vec3& origin, const Vec3& direction) :
    origin_vec(origin), direction_vec(Math::normalize(direction)), inv_direction_vec(1.0f / direction_vec)
{}

const Vec3& Ray::origin() const
{
	return origin_vec;
}

const Vec3& Ray::direction() const
{
	return direction_vec;
}

const Vec3& Ray::invDirection() const
{
	return inv_direction_vec;
}

}        // namespace Vortex
