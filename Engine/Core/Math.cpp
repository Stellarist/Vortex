module Core.Math;

import glm;

namespace Vortex::Math {

float radians(float degrees)
{
	return glm::radians(degrees);
}

Vec3 radians(const Vec3& degrees)
{
	return glm::radians(degrees);
}

float degrees(float radians)
{
	return glm::degrees(radians);
}

Vec3 degrees(const Vec3& radians)
{
	return glm::degrees(radians);
}

Vec3 eulerAngles(const Quat& rotation)
{
	return glm::gtc::eulerAngles(rotation);
}

Quat fromEuler(const Vec3& euler_angles)
{
	return glm::quat(euler_angles);
}

Quat angleAxis(float angle, const Vec3& axis)
{
	return glm::gtc::angleAxis(angle, axis);
}

float clamp(float value, float min, float max)
{
	return glm::clamp(value, min, max);
}

Vec3 min(const Vec3& lhs, const Vec3& rhs)
{
	return glm::min(lhs, rhs);
}

Vec3 max(const Vec3& lhs, const Vec3& rhs)
{
	return glm::max(lhs, rhs);
}

Vec3 normalize(const Vec3& vector)
{
	return glm::normalize(vector);
}

Quat normalize(const Quat& quaternion)
{
	return glm::normalize(quaternion);
}

Mat4 inverse(const Mat4& matrix)
{
	return glm::inverse(matrix);
}

Mat4 perspective(float fov, float aspect_ratio, float near_plane, float far_plane)
{
	return glm::gtc::perspectiveRH_ZO(fov, aspect_ratio, near_plane, far_plane);
}

Mat4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	return glm::gtc::orthoRH_ZO(left, right, bottom, top, near_plane, far_plane);
}

Mat4 translate(const Mat4& matrix, const Vec3& translation)
{
	return glm::gtc::translate(matrix, translation);
}

Mat4 scale(const Mat4& matrix, const Vec3& scaling)
{
	return glm::gtc::scale(matrix, scaling);
}

Mat4 toMat4(const Quat& quaternion)
{
	return glm::gtc::mat4_cast(quaternion);
}

}        // namespace Vortex::Math
