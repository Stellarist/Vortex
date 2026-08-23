module Runtime.World;

namespace Vortex {

CameraComponent::CameraComponent(std::string component_name) :
    SceneComponent(std::move(component_name))
{}

Mat4 CameraComponent::getView() const noexcept
{
	return Math::inverse(getWorldMatrix());
}

Mat4 CameraComponent::getPreRotation() const noexcept
{
	return pre_rotation;
}

CameraComponent& CameraComponent::setPreRotation(const Mat4& new_pre_rotation) noexcept
{
	pre_rotation = new_pre_rotation;
	return *this;
}


PerspectiveCameraComponent::PerspectiveCameraComponent(std::string component_name) :
    CameraComponent(std::move(component_name))
{}

PerspectiveCameraComponent::PerspectiveCameraComponent(
    std::string component_name,
    float       new_fov,
    float       new_aspect_ratio,
    float       new_near_plane,
    float       new_far_plane) :
    CameraComponent(std::move(component_name)),
    aspect_ratio(new_aspect_ratio),
    fov(new_fov),
    far_plane(new_far_plane),
    near_plane(new_near_plane)
{}

float PerspectiveCameraComponent::getFarPlane() const noexcept
{
	return far_plane;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setFarPlane(float new_far_plane) noexcept
{
	far_plane = new_far_plane;
	return *this;
}

float PerspectiveCameraComponent::getNearPlane() const noexcept
{
	return near_plane;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setNearPlane(float new_near_plane) noexcept
{
	near_plane = new_near_plane;
	return *this;
}

float PerspectiveCameraComponent::getAspectRatio() const noexcept
{
	return aspect_ratio;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setAspectRatio(float new_aspect_ratio) noexcept
{
	aspect_ratio = new_aspect_ratio;
	return *this;
}

float PerspectiveCameraComponent::getFov() const noexcept
{
	return fov;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setFov(float new_fov) noexcept
{
	fov = new_fov;
	return *this;
}

Vec3 PerspectiveCameraComponent::getFront() const noexcept
{
	return Math::normalize(Vec3(getWorldMatrix() * Vec4(0.0f, 0.0f, -1.0f, 0.0f)));
}

Vec3 PerspectiveCameraComponent::getUp() const noexcept
{
	return Math::normalize(Vec3(getWorldMatrix() * Vec4(0.0f, 1.0f, 0.0f, 0.0f)));
}

Vec3 PerspectiveCameraComponent::getRight() const noexcept
{
	return Math::normalize(Vec3(getWorldMatrix() * Vec4(1.0f, 0.0f, 0.0f, 0.0f)));
}

Mat4 PerspectiveCameraComponent::getProjection() const noexcept
{
	auto projection = Math::perspective(fov, aspect_ratio, near_plane, far_plane);
	projection[1][1] *= -1.0f;
	return projection;
}


OrthographicCameraComponent::OrthographicCameraComponent(std::string component_name) :
    CameraComponent(std::move(component_name))
{}

OrthographicCameraComponent::OrthographicCameraComponent(
    std::string component_name,
    float       new_left,
    float       new_right,
    float       new_bottom,
    float       new_top,
    float       new_near_plane,
    float       new_far_plane) :
    CameraComponent(std::move(component_name)),
    left(new_left),
    right(new_right),
    top(new_top),
    bottom(new_bottom),
    near_plane(new_near_plane),
    far_plane(new_far_plane)
{}

float OrthographicCameraComponent::getLeft() const noexcept
{
	return left;
}

OrthographicCameraComponent& OrthographicCameraComponent::setLeft(float value) noexcept
{
	left = value;
	return *this;
}

float OrthographicCameraComponent::getRight() const noexcept
{
	return right;
}

OrthographicCameraComponent& OrthographicCameraComponent::setRight(float value) noexcept
{
	right = value;
	return *this;
}

float OrthographicCameraComponent::getTop() const noexcept
{
	return top;
}

OrthographicCameraComponent& OrthographicCameraComponent::setTop(float value) noexcept
{
	top = value;
	return *this;
}

float OrthographicCameraComponent::getBottom() const noexcept
{
	return bottom;
}

OrthographicCameraComponent& OrthographicCameraComponent::setBottom(float value) noexcept
{
	bottom = value;
	return *this;
}

float OrthographicCameraComponent::getNearPlane() const noexcept
{
	return near_plane;
}

OrthographicCameraComponent& OrthographicCameraComponent::setNearPlane(float value) noexcept
{
	near_plane = value;
	return *this;
}

float OrthographicCameraComponent::getFarPlane() const noexcept
{
	return far_plane;
}

OrthographicCameraComponent& OrthographicCameraComponent::setFarPlane(float value) noexcept
{
	far_plane = value;
	return *this;
}

Mat4 OrthographicCameraComponent::getProjection() const noexcept
{
	auto projection = Math::orthographic(left, right, bottom, top, near_plane, far_plane);
	projection[1][1] *= -1.0f;
	return projection;
}

}        // namespace Vortex
