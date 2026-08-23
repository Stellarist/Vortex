export module Runtime.World:AABB;

import Core;
import :Ray;

export namespace Vortex {

struct AABB {
private:
	Vec3 min_bound;
	Vec3 max_bound;

public:
	AABB() noexcept;
	AABB(const Vec3& min, const Vec3& max) noexcept;

	Vec3 min() const noexcept;
	Vec3 max() const noexcept;

	Vec3 center() const noexcept;
	Vec3 scale() const noexcept;

	float area() const noexcept;
	float volume() const noexcept;

	void expand(const Vec3& point) noexcept;
	void expand(const AABB& other) noexcept;
	void expand(std::span<const Vec3> points) noexcept;

	bool intersects(const AABB& other) const noexcept;
	bool intersects(const Ray& ray, float& tmin, float& tmax) const noexcept;

	bool contains(const Vec3& point) const noexcept;
	bool contains(const AABB& other) const noexcept;

	void reset() noexcept;
	void transform(const Mat4& matrix) noexcept;

	bool valid() const noexcept;
};

}        // namespace Vortex
