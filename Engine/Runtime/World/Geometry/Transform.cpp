module Runtime.World;

namespace Vortex {

Transform& Transform::translate(const Vec3& delta) noexcept
{
	translation += delta;
	return *this;
}

Transform& Transform::rotate(const Vec3& axis, float angle) noexcept
{
	rotation = Math::normalize(Math::angleAxis(angle, axis) * rotation);
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
	rotation = Math::normalize(new_rotation);
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

Transform& Transform::setMatrix(const Mat4& matrix) noexcept
{
	Math::decomposeTransform(matrix, translation, rotation, scaling);
	return *this;
}

}        // namespace Vortex
