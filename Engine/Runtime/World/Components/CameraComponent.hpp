export module Runtime.World:CameraComponent;

import Core;
import :SceneComponent;

export namespace Vortex {

class CameraComponent : public SceneComponent {
protected:
	Mat4 pre_rotation{1.0f};

public:
	CameraComponent(std::string name);
	~CameraComponent() override = default;

	virtual Mat4 getProjection() const noexcept = 0;
	Mat4         getView() const noexcept;

	Mat4 getPreRotation() const noexcept;
	auto setPreRotation(const Mat4& pre_rotation) noexcept -> CameraComponent&;
};


class PerspectiveCameraComponent : public CameraComponent {
private:
	float aspect_ratio{1.0f};
	float fov{0.785f};
	float far_plane{100.0f};
	float near_plane{0.1f};

public:
	PerspectiveCameraComponent(std::string name);
	PerspectiveCameraComponent(std::string name, float fov, float aspect_ratio, float near_plane, float far_plane);
	~PerspectiveCameraComponent() override = default;

	float getFarPlane() const noexcept;
	auto  setFarPlane(float far_plane) noexcept -> PerspectiveCameraComponent&;

	float getNearPlane() const noexcept;
	auto  setNearPlane(float near_plane) noexcept -> PerspectiveCameraComponent&;

	float getAspectRatio() const noexcept;
	auto  setAspectRatio(float aspect_ratio) noexcept -> PerspectiveCameraComponent&;

	float getFov() const noexcept;
	auto  setFov(float fov) noexcept -> PerspectiveCameraComponent&;

	Vec3 getFront() const noexcept;
	Vec3 getUp() const noexcept;
	Vec3 getRight() const noexcept;

	Mat4 getProjection() const noexcept override;
};


class OrthographicCameraComponent : public CameraComponent {
private:
	float left{-1.0f};
	float right{1.0f};
	float top{1.0f};
	float bottom{-1.0f};
	float near_plane{0.0f};
	float far_plane{1.0f};

public:
	OrthographicCameraComponent(std::string name);
	OrthographicCameraComponent(std::string name, float left, float right, float bottom, float top, float near_plane, float far_plane);
	~OrthographicCameraComponent() override = default;

	float getLeft() const noexcept;
	auto  setLeft(float left) noexcept -> OrthographicCameraComponent&;

	float getRight() const noexcept;
	auto  setRight(float right) noexcept -> OrthographicCameraComponent&;

	float getTop() const noexcept;
	auto  setTop(float top) noexcept -> OrthographicCameraComponent&;

	float getBottom() const noexcept;
	auto  setBottom(float bottom) noexcept -> OrthographicCameraComponent&;

	float getNearPlane() const noexcept;
	auto  setNearPlane(float near_plane) noexcept -> OrthographicCameraComponent&;

	float getFarPlane() const noexcept;
	auto  setFarPlane(float far_plane) noexcept -> OrthographicCameraComponent&;

	Mat4 getProjection() const noexcept override;
};

}        // namespace Vortex
