export module Runtime.World:Geometry.Ray;

import Core;

export namespace Vortex {

class Ray {
private:
	Vec3 origin_vec;
	Vec3 direction_vec;
	Vec3 inv_direction_vec;

public:
	Ray(const Vec3& origin, const Vec3& direction) noexcept;

	const Vec3& origin() const noexcept;
	const Vec3& direction() const noexcept;
	const Vec3& invDirection() const noexcept;
};

}        // namespace Vortex
