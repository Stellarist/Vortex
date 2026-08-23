export module Core.Math;

import glm;

export namespace Vortex {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;
using Quat = glm::quat;

namespace Math {

float radians(float degrees);
Vec3  radians(const Vec3& degrees);

float degrees(float radians);
Vec3  degrees(const Vec3& radians);

Vec3 eulerAngles(const Quat& rotation);
Quat fromEuler(const Vec3& euler_angles);
Quat angleAxis(float angle, const Vec3& axis);

float clamp(float value, float min, float max);
Vec3  min(const Vec3& lhs, const Vec3& rhs);
Vec3  max(const Vec3& lhs, const Vec3& rhs);

Vec3 normalize(const Vec3& vector);
Quat normalize(const Quat& quaternion);

Mat4 inverse(const Mat4& matrix);
Mat4 translate(const Mat4& matrix, const Vec3& translation);
Mat4 scale(const Mat4& matrix, const Vec3& scaling);
Mat4 perspective(float fov, float aspect_ratio, float near_plane, float far_plane);
Mat4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);

Mat4 composeTransform(const Vec3& translation, const Quat& rotation, const Vec3& scaling);
bool decomposeTransform(const Mat4& matrix, Vec3& translation, Quat& rotation, Vec3& scaling);

}        // namespace Math

}        // namespace Vortex
