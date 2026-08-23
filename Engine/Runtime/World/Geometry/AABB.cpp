module Runtime.World;

namespace Vortex {

AABB::AABB() noexcept
{
	reset();
}

AABB::AABB(const Vec3& min, const Vec3& max) noexcept :
    min_bound(min), max_bound(max)
{}

Vec3 AABB::min() const noexcept
{
	return min_bound;
}

Vec3 AABB::max() const noexcept
{
	return max_bound;
}

Vec3 AABB::center() const noexcept
{
	return (min_bound + max_bound) * 0.5f;
}

Vec3 AABB::scale() const noexcept
{
	return max_bound - min_bound;
}

float AABB::area() const noexcept
{
	Vec3 extent = scale();
	return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
}

float AABB::volume() const noexcept
{
	Vec3 extent = scale();
	return extent.x * extent.y * extent.z;
}

void AABB::expand(const Vec3& point) noexcept
{
	min_bound = Math::min(min_bound, point);
	max_bound = Math::max(max_bound, point);
}

void AABB::expand(const AABB& other) noexcept
{
	expand(other.min());
	expand(other.max());
}

void AABB::expand(std::span<const Vec3> points) noexcept
{
	for (const auto& point : points)
		expand(point);
}

bool AABB::intersects(const AABB& other) const noexcept
{
	if (!valid() || !other.valid())
		return false;

	return (min_bound.x <= other.max().x && max_bound.x >= other.min().x) && (min_bound.y <= other.max().y && max_bound.y >= other.min().y) && (min_bound.z <= other.max().z && max_bound.z >= other.min().z);
}

bool AABB::intersects(const Ray& ray, float& tmin, float& tmax) const noexcept
{
	if (!valid())
		return false;

	float tx1 = (min_bound.x - ray.origin().x) * ray.invDirection().x;
	float tx2 = (max_bound.x - ray.origin().x) * ray.invDirection().x;

	float ty1 = (min_bound.y - ray.origin().y) * ray.invDirection().y;
	float ty2 = (max_bound.y - ray.origin().y) * ray.invDirection().y;

	float tz1 = (min_bound.z - ray.origin().z) * ray.invDirection().z;
	float tz2 = (max_bound.z - ray.origin().z) * ray.invDirection().z;

	tmin = std::max({std::min(tx1, tx2), std::min(ty1, ty2), std::min(tz1, tz2)});
	tmax = std::min({std::max(tx1, tx2), std::max(ty1, ty2), std::max(tz1, tz2)});

	return tmax >= tmin && tmax >= 0.0f;
}

bool AABB::contains(const Vec3& point) const noexcept
{
	return (point.x >= min_bound.x && point.x <= max_bound.x) && (point.y >= min_bound.y && point.y <= max_bound.y) && (point.z >= min_bound.z && point.z <= max_bound.z);
}

bool AABB::contains(const AABB& other) const noexcept
{
	return contains(other.min()) && contains(other.max());
}

void AABB::reset() noexcept
{
	min_bound = Vec3(std::numeric_limits<float>::max());
	max_bound = Vec3(std::numeric_limits<float>::lowest());
}

void AABB::transform(const Mat4& matrix) noexcept
{
	if (!valid())
		return;

	AABB result;
	result.expand(Vec3(matrix * Vec4(min_bound.x, min_bound.y, min_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(max_bound.x, min_bound.y, min_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(min_bound.x, max_bound.y, min_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(max_bound.x, max_bound.y, min_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(min_bound.x, min_bound.y, max_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(max_bound.x, min_bound.y, max_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(min_bound.x, max_bound.y, max_bound.z, 1.0f)));
	result.expand(Vec3(matrix * Vec4(max_bound.x, max_bound.y, max_bound.z, 1.0f)));

	*this = result;
}

bool AABB::valid() const noexcept
{
	return (min_bound.x <= max_bound.x) && (min_bound.y <= max_bound.y) && (min_bound.z <= max_bound.z);
}

}        // namespace Vortex
