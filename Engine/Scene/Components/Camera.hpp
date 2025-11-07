#pragma once

#include <string>

#include <glm/glm.hpp>

#include "Scene/Core/Component.hpp"

class Camera : public Component {
protected:
	glm::mat4 pre_rotation{1.0f};

public:
	Camera(const std::string& name);
	~Camera() override = default;

	std::type_index getType() override;

	virtual glm::mat4 getProjection() = 0;

	auto getView() -> glm::mat4;

	glm::mat4 getPreRotation() const;
	void      setPreRotation(const glm::mat4& pre_rotation);
};

class PerspectiveCamera : public Camera {
private:
	float aspect_ratio{1.0f};
	float fov{0.785f};
	float far_plane{100.0f};
	float near_plane{0.1f};

public:
	PerspectiveCamera(const std::string& name);
	PerspectiveCamera(const std::string& name, float fov, float aspect_ratio, float near_plane, float far_plane);
	~PerspectiveCamera() override = default;

	std::type_index getType() override;

	float getFarPlane() const;
	void  setFarPlane(float zfar);

	float getNearPlane() const;
	void  setNearPlane(float znear);

	float getAspectRatio() const;
	void  setAspectRatio(float aspect_ratio);

	float getFov() const;
	void  setFov(float fov);

	glm::vec3 getFront();
	glm::vec3 getUp();
	glm::vec3 getRight();

	glm::mat4 getProjection() override;
};

class OrthoCamera : public Camera {
private:
	float left{-1.0f};
	float right{1.0f};
	float top{1.0f};
	float bottom{-1.0f};
	float near_plane{0.0f};
	float far_plane{1.0f};

public:
	OrthoCamera(const std::string& name);
	OrthoCamera(const std::string& name, float left, float right, float bottom, float top, float near_plane, float far_plane);

	OrthoCamera(const OrthoCamera&) = delete;
	OrthoCamera& operator=(const OrthoCamera&) = delete;

	OrthoCamera(OrthoCamera&&) noexcept = default;
	OrthoCamera& operator=(OrthoCamera&&) noexcept = default;

	~OrthoCamera() override = default;

	std::type_index getType() override;

	float getLeft() const;
	void  setLeft(float left);

	float getRight() const;
	void  setRight(float right);

	float getTop() const;
	void  setTop(float top);

	float getBottom() const;
	void  setBottom(float bottom);

	float getNearPlane() const;
	void  setNearPlane(float near_plane);

	float getFarPlane() const;
	void  setFarPlane(float far_plane);

	glm::mat4 getProjection() override;
};
