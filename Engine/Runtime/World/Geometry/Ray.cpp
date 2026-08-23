module Runtime.World;

namespace Vortex {

Ray::Ray(const Vec3& origin, const Vec3& direction) noexcept :
    origin_vec(origin), direction_vec(Math::normalize(direction)), inv_direction_vec(1.0f / direction_vec)
{}

const Vec3& Ray::origin() const noexcept
{
	return origin_vec;
}

const Vec3& Ray::direction() const noexcept
{
	return direction_vec;
}

const Vec3& Ray::invDirection() const noexcept
{
	return inv_direction_vec;
}

}        // namespace Vortex
