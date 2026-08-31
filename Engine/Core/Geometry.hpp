export module Core:Geometry;

import std;
import :Types;
import :Math;

export namespace Vortex {

class Transform {
private:
	Vec3 translation{0.0f, 0.0f, 0.0f};
	Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
	Vec3 scaling{1.0f, 1.0f, 1.0f};

public:
	auto translate(const Vec3& delta) noexcept -> Transform&;
	auto rotate(const Vec3& axis, float angle) noexcept -> Transform&;
	auto scale(const Vec3& factor) noexcept -> Transform&;

	auto getTranslation() const noexcept -> const Vec3&;
	auto setTranslation(const Vec3& translation) noexcept -> Transform&;

	auto getRotation() const noexcept -> const Quat&;
	auto setRotation(const Quat& rotation) noexcept -> Transform&;

	auto getScaling() const noexcept -> const Vec3&;
	auto setScaling(const Vec3& scaling) noexcept -> Transform&;

	Mat4 getMatrix() const noexcept;
	bool setMatrix(const Mat4& matrix) noexcept;
};

class Ray {
private:
	Vec3 origin_vec{};
	Vec3 direction_vec{};
	Vec3 inv_direction_vec{};

public:
	Ray(const Vec3& origin, const Vec3& direction) noexcept;

	const Vec3& origin() const noexcept;
	const Vec3& direction() const noexcept;
	const Vec3& invDirection() const noexcept;
	bool valid() const noexcept;
};

struct Bounds {
private:
	Vec3 min_bound{};
	Vec3 max_bound{};

public:
	Bounds() noexcept;
	Bounds(const Vec3& min, const Vec3& max) noexcept;

	Vec3 min() const noexcept;
	Vec3 max() const noexcept;

	Vec3 center() const noexcept;
	Vec3 size() const noexcept;

	float area() const noexcept;
	float volume() const noexcept;

	void expand(const Vec3& point) noexcept;
	void expand(const Bounds& other) noexcept;
	void expand(std::span<const Vec3> points) noexcept;

	bool intersects(const Bounds& other) const noexcept;
	bool intersects(const Ray& ray, float& tmin, float& tmax) const noexcept;

	bool contains(const Vec3& point) const noexcept;
	bool contains(const Bounds& other) const noexcept;

	void reset() noexcept;
	void transform(const Mat4& matrix) noexcept;

	bool valid() const noexcept;
};

}        // namespace Vortex
