export module Runtime.World:Transform;

import Core;

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
	auto setMatrix(const Mat4& matrix) noexcept -> Transform&;
};

}        // namespace Vortex
