export module Runtime.World:AABB;

import Core;
import :Ray;

export namespace Vortex {

struct AABB {
private:
	Vec3 min_bound;
	Vec3 max_bound;

public:
	AABB();
	AABB(const Vec3& min, const Vec3& max);

	Vec3 min() const;
	Vec3 max() const;

	Vec3 center() const;
	Vec3 scale() const;

	float area() const;
	float volume() const;

	void expand(const Vec3& point);
	void expand(const AABB& other);
	void expand(std::span<const Vec3> points);

	bool intersects(const AABB& other) const;
	bool intersects(const Ray& ray, float& tmin, float& tmax) const;

	bool contains(const Vec3& point) const;
	bool contains(const AABB& other) const;

	void reset();
	void transform(const Mat4& matrix);

	bool valid() const;
};

}        // namespace Vortex
