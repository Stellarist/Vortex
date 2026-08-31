module Core;

namespace Vortex {

Transform& Transform::translate(const Vec3& delta) noexcept
{
	translation += delta;
	return *this;
}

Transform& Transform::rotate(const Vec3& axis, float angle) noexcept
{
	if (!Math::isFinite(angle))
		return *this;
	const Vec3 normalized_axis = Math::safeNormalize(axis);
	if (Math::isNearlyZero(normalized_axis))
		return *this;
	rotation = Math::safeNormalize(Math::angleAxis(angle, normalized_axis) * rotation);
	return *this;
}

Transform& Transform::scale(const Vec3& factor) noexcept
{
	scaling *= factor;
	return *this;
}

const Vec3& Transform::getTranslation() const noexcept
{
	return translation;
}

Transform& Transform::setTranslation(const Vec3& new_translation) noexcept
{
	translation = new_translation;
	return *this;
}

const Quat& Transform::getRotation() const noexcept
{
	return rotation;
}

Transform& Transform::setRotation(const Quat& new_rotation) noexcept
{
	rotation = Math::safeNormalize(new_rotation);
	return *this;
}

const Vec3& Transform::getScaling() const noexcept
{
	return scaling;
}

Transform& Transform::setScaling(const Vec3& new_scaling) noexcept
{
	scaling = new_scaling;
	return *this;
}

Mat4 Transform::getMatrix() const noexcept
{
	return Math::composeTransform(translation, rotation, scaling);
}

bool Transform::setMatrix(const Mat4& matrix) noexcept
{
	Vec3 new_translation{};
	Quat new_rotation{};
	Vec3 new_scaling{};
	if (!Math::decomposeTransform(matrix, new_translation, new_rotation, new_scaling) ||
	    !Math::isFinite(new_translation) || !Math::isFinite(new_rotation) || !Math::isFinite(new_scaling))
		return false;

	translation = new_translation;
	rotation = Math::safeNormalize(new_rotation);
	scaling = new_scaling;
	return true;
}


Ray::Ray(const Vec3& origin, const Vec3& direction) noexcept :
    origin_vec(origin), direction_vec(Math::safeNormalize(direction))
{
	const float infinity = std::numeric_limits<float>::infinity();
	inv_direction_vec = Vec3{
	    direction_vec.x == 0.0f ? infinity : 1.0f / direction_vec.x,
	    direction_vec.y == 0.0f ? infinity : 1.0f / direction_vec.y,
	    direction_vec.z == 0.0f ? infinity : 1.0f / direction_vec.z,
	};
}

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

bool Ray::valid() const noexcept
{
	return !Math::isNearlyZero(direction_vec);
}


Bounds::Bounds() noexcept
{
	reset();
}

Bounds::Bounds(const Vec3& min, const Vec3& max) noexcept :
    min_bound(min), max_bound(max)
{}

Vec3 Bounds::min() const noexcept
{
	return min_bound;
}

Vec3 Bounds::max() const noexcept
{
	return max_bound;
}

Vec3 Bounds::center() const noexcept
{
	if (!valid())
		return Vec3{};
	return (min_bound + max_bound) * 0.5f;
}

Vec3 Bounds::size() const noexcept
{
	if (!valid())
		return Vec3{};
	return max_bound - min_bound;
}

float Bounds::area() const noexcept
{
	if (!valid())
		return 0.0f;
	const Vec3 extent = size();
	return 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
}

float Bounds::volume() const noexcept
{
	if (!valid())
		return 0.0f;
	const Vec3 extent = size();
	return extent.x * extent.y * extent.z;
}

void Bounds::expand(const Vec3& point) noexcept
{
	min_bound = Math::min(min_bound, point);
	max_bound = Math::max(max_bound, point);
}

void Bounds::expand(const Bounds& other) noexcept
{
	if (!other.valid())
		return;
	expand(other.min());
	expand(other.max());
}

void Bounds::expand(std::span<const Vec3> points) noexcept
{
	for (const auto& point : points)
		expand(point);
}

bool Bounds::intersects(const Bounds& other) const noexcept
{
	if (!valid() || !other.valid())
		return false;

	return (min_bound.x <= other.max().x && max_bound.x >= other.min().x) &&
	    (min_bound.y <= other.max().y && max_bound.y >= other.min().y) &&
	    (min_bound.z <= other.max().z && max_bound.z >= other.min().z);
}

bool Bounds::intersects(const Ray& ray, float& tmin, float& tmax) const noexcept
{
	if (!valid() || !ray.valid())
		return false;

	tmin = -std::numeric_limits<float>::infinity();
	tmax = std::numeric_limits<float>::infinity();
	for (uint32 axis = 0; axis < 3; ++axis) {
		const float direction = ray.direction()[axis];
		const float origin = ray.origin()[axis];
		if (Math::isNearlyZero(direction, std::numeric_limits<float>::epsilon())) {
			if (origin < min_bound[axis] || origin > max_bound[axis])
				return false;
			continue;
		}

		float near_value = (min_bound[axis] - origin) / direction;
		float far_value = (max_bound[axis] - origin) / direction;
		if (near_value > far_value)
			std::swap(near_value, far_value);
		tmin = std::max(tmin, near_value);
		tmax = std::min(tmax, far_value);
		if (tmin > tmax)
			return false;
	}

	return tmax >= 0.0f;
}

bool Bounds::contains(const Vec3& point) const noexcept
{
	if (!valid())
		return false;
	return (point.x >= min_bound.x && point.x <= max_bound.x) &&
	    (point.y >= min_bound.y && point.y <= max_bound.y) &&
	    (point.z >= min_bound.z && point.z <= max_bound.z);
}

bool Bounds::contains(const Bounds& other) const noexcept
{
	if (!valid() || !other.valid())
		return false;
	return contains(other.min()) && contains(other.max());
}

void Bounds::reset() noexcept
{
	min_bound = Vec3(std::numeric_limits<float>::max());
	max_bound = Vec3(std::numeric_limits<float>::lowest());
}

void Bounds::transform(const Mat4& matrix) noexcept
{
	if (!valid())
		return;

	Bounds result;
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

bool Bounds::valid() const noexcept
{
	return Math::isFinite(min_bound) && Math::isFinite(max_bound) &&
	    min_bound.x <= max_bound.x && min_bound.y <= max_bound.y && min_bound.z <= max_bound.z;
}

}        // namespace Vortex
