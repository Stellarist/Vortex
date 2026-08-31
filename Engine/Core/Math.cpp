module Core;

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

float dot(const Vec3& lhs, const Vec3& rhs)
{
	return glm::dot(lhs, rhs);
}

Vec3 cross(const Vec3& lhs, const Vec3& rhs)
{
	return glm::cross(lhs, rhs);
}

float length(const Vec3& vector)
{
	return glm::length(vector);
}

float lengthSquared(const Vec3& vector)
{
	return glm::dot(vector, vector);
}

float distance(const Vec3& lhs, const Vec3& rhs)
{
	return glm::distance(lhs, rhs);
}

Vec3 normalize(const Vec3& vector)
{
	return glm::normalize(vector);
}

Quat normalize(const Quat& quaternion)
{
	return glm::normalize(quaternion);
}

Vec3 safeNormalize(const Vec3& vector, const Vec3& fallback)
{
	const float squared_length = lengthSquared(vector);
	if (isFinite(squared_length) && squared_length > std::numeric_limits<float>::epsilon())
		return vector / std::sqrt(squared_length);

	const float fallback_squared_length = lengthSquared(fallback);
	if (isFinite(fallback_squared_length) && fallback_squared_length > std::numeric_limits<float>::epsilon())
		return fallback / std::sqrt(fallback_squared_length);
	return Vec3{};
}

Quat safeNormalize(const Quat& quaternion, const Quat& fallback)
{
	const float squared_length = glm::dot(quaternion, quaternion);
	if (isFinite(quaternion) && isFinite(squared_length) && squared_length > std::numeric_limits<float>::epsilon())
		return quaternion / std::sqrt(squared_length);

	const float fallback_squared_length = glm::dot(fallback, fallback);
	if (isFinite(fallback) && isFinite(fallback_squared_length) && fallback_squared_length > std::numeric_limits<float>::epsilon())
		return fallback / std::sqrt(fallback_squared_length);
	return Quat{1.0f, 0.0f, 0.0f, 0.0f};
}

bool isFinite(float value) noexcept
{
	return std::isfinite(value);
}

bool isFinite(const Vec2& vector) noexcept
{
	return isFinite(vector.x) && isFinite(vector.y);
}

bool isFinite(const Vec3& vector) noexcept
{
	return isFinite(vector.x) && isFinite(vector.y) && isFinite(vector.z);
}

bool isFinite(const Vec4& vector) noexcept
{
	return isFinite(vector.x) && isFinite(vector.y) && isFinite(vector.z) && isFinite(vector.w);
}

bool isFinite(const Quat& quaternion) noexcept
{
	return isFinite(quaternion.w) && isFinite(quaternion.x) && isFinite(quaternion.y) && isFinite(quaternion.z);
}

bool isFinite(const Mat4& matrix) noexcept
{
	return isFinite(matrix[0]) && isFinite(matrix[1]) && isFinite(matrix[2]) && isFinite(matrix[3]);
}

bool nearlyEqual(float lhs, float rhs, float epsilon) noexcept
{
	return isFinite(lhs) && isFinite(rhs) && isFinite(epsilon) && epsilon >= 0.0f && std::abs(lhs - rhs) <= epsilon;
}

bool nearlyEqual(const Vec3& lhs, const Vec3& rhs, float epsilon) noexcept
{
	return nearlyEqual(lhs.x, rhs.x, epsilon) && nearlyEqual(lhs.y, rhs.y, epsilon) && nearlyEqual(lhs.z, rhs.z, epsilon);
}

bool isNearlyZero(float value, float epsilon) noexcept
{
	return nearlyEqual(value, 0.0f, epsilon);
}

bool isNearlyZero(const Vec3& vector, float epsilon) noexcept
{
	return nearlyEqual(vector, Vec3{}, epsilon);
}

Mat4 inverse(const Mat4& matrix)
{
	return glm::inverse(matrix);
}

Mat4 transpose(const Mat4& matrix)
{
	return glm::transpose(matrix);
}

Mat4 translate(const Mat4& matrix, const Vec3& translation)
{
	return glm::gtc::translate(matrix, translation);
}

Mat4 scale(const Mat4& matrix, const Vec3& scaling)
{
	return glm::gtc::scale(matrix, scaling);
}

Mat4 perspective(float fov, float aspect_ratio, float near_plane, float far_plane)
{
	return glm::gtc::perspectiveRH_ZO(fov, aspect_ratio, near_plane, far_plane);
}

Mat4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	return glm::gtc::orthoRH_ZO(left, right, bottom, top, near_plane, far_plane);
}

Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
	return glm::gtc::lookAtRH(eye, center, up);
}

Mat4 composeTransform(const Vec3& translation, const Quat& rotation, const Vec3& scaling)
{
	return glm::gtc::translate(Mat4(1.0f), translation) * glm::gtc::mat4_cast(glm::normalize(rotation)) * glm::gtc::scale(Mat4(1.0f), scaling);
}

bool decomposeTransform(const Mat4& matrix, Vec3& translation, Quat& rotation, Vec3& scaling)
{
	Vec3 skew;
	Vec4 perspective;
	if (!glm::gtx::decompose(matrix, scaling, rotation, translation, skew, perspective))
		return false;

	rotation = glm::normalize(rotation);
	return true;
}

}        // namespace Vortex::Math
