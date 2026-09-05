module Runtime.World;

namespace Vortex {

static constexpr float minimum_plane = 0.001f;
static constexpr float minimum_extent = 0.001f;
static constexpr float minimum_fov = 0.0174532925f;
static constexpr float maximum_fov = 3.12413936f;

static float finiteOr(float value, float fallback) noexcept
{
	return std::isfinite(value) ? value : fallback;
}

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
    float new_fov,
    float new_aspect_ratio,
    float new_near_plane,
    float new_far_plane) :
    CameraComponent(std::move(component_name))
{
	setAspectRatio(new_aspect_ratio);
	setFov(new_fov);
	setNearPlane(new_near_plane);
	setFarPlane(new_far_plane);
}

float PerspectiveCameraComponent::getFarPlane() const noexcept
{
	return far_plane;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setFarPlane(float new_far_plane) noexcept
{
	far_plane = std::max(finiteOr(new_far_plane, near_plane + 100.0f), near_plane + minimum_plane);
	return *this;
}

float PerspectiveCameraComponent::getNearPlane() const noexcept
{
	return near_plane;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setNearPlane(float new_near_plane) noexcept
{
	near_plane = std::max(finiteOr(new_near_plane, 0.1f), minimum_plane);
	far_plane = std::max(far_plane, near_plane + minimum_plane);
	return *this;
}

float PerspectiveCameraComponent::getAspectRatio() const noexcept
{
	return aspect_ratio;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setAspectRatio(float new_aspect_ratio) noexcept
{
	aspect_ratio = std::max(finiteOr(new_aspect_ratio, 1.0f), minimum_extent);
	return *this;
}

float PerspectiveCameraComponent::getFov() const noexcept
{
	return fov;
}

PerspectiveCameraComponent& PerspectiveCameraComponent::setFov(float new_fov) noexcept
{
	fov = std::clamp(finiteOr(new_fov, 0.785f), minimum_fov, maximum_fov);
	return *this;
}

Vec3 PerspectiveCameraComponent::getFront() const noexcept
{
	return Math::safeNormalize(Vec3(getWorldMatrix() * Vec4(0.0f, 0.0f, -1.0f, 0.0f)));
}

Vec3 PerspectiveCameraComponent::getUp() const noexcept
{
	return Math::safeNormalize(Vec3(getWorldMatrix() * Vec4(0.0f, 1.0f, 0.0f, 0.0f)));
}

Vec3 PerspectiveCameraComponent::getRight() const noexcept
{
	return Math::safeNormalize(Vec3(getWorldMatrix() * Vec4(1.0f, 0.0f, 0.0f, 0.0f)));
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
    float new_left,
    float new_right,
    float new_bottom,
    float new_top,
    float new_near_plane,
    float new_far_plane) :
    CameraComponent(std::move(component_name))
{
	setLeft(new_left);
	setRight(new_right);
	setBottom(new_bottom);
	setTop(new_top);
	setNearPlane(new_near_plane);
	setFarPlane(new_far_plane);
}

float OrthographicCameraComponent::getLeft() const noexcept
{
	return left;
}

OrthographicCameraComponent& OrthographicCameraComponent::setLeft(float value) noexcept
{
	left = finiteOr(value, -1.0f);
	if (right <= left)
		right = left + minimum_extent;
	return *this;
}

float OrthographicCameraComponent::getRight() const noexcept
{
	return right;
}

OrthographicCameraComponent& OrthographicCameraComponent::setRight(float value) noexcept
{
	right = finiteOr(value, 1.0f);
	if (right <= left)
		left = right - minimum_extent;
	return *this;
}

float OrthographicCameraComponent::getTop() const noexcept
{
	return top;
}

OrthographicCameraComponent& OrthographicCameraComponent::setTop(float value) noexcept
{
	top = finiteOr(value, 1.0f);
	if (top <= bottom)
		bottom = top - minimum_extent;
	return *this;
}

float OrthographicCameraComponent::getBottom() const noexcept
{
	return bottom;
}

OrthographicCameraComponent& OrthographicCameraComponent::setBottom(float value) noexcept
{
	bottom = finiteOr(value, -1.0f);
	if (top <= bottom)
		top = bottom + minimum_extent;
	return *this;
}

float OrthographicCameraComponent::getNearPlane() const noexcept
{
	return near_plane;
}

OrthographicCameraComponent& OrthographicCameraComponent::setNearPlane(float value) noexcept
{
	near_plane = std::max(finiteOr(value, minimum_plane), minimum_plane);
	far_plane = std::max(far_plane, near_plane + minimum_plane);
	return *this;
}

float OrthographicCameraComponent::getFarPlane() const noexcept
{
	return far_plane;
}

OrthographicCameraComponent& OrthographicCameraComponent::setFarPlane(float value) noexcept
{
	far_plane = std::max(finiteOr(value, near_plane + 1.0f), near_plane + minimum_plane);
	return *this;
}

Mat4 OrthographicCameraComponent::getProjection() const noexcept
{
	auto projection = Math::orthographic(left, right, bottom, top, near_plane, far_plane);
	projection[1][1] *= -1.0f;
	return projection;
}

}        // namespace Vortex
