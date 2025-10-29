#pragma once

#include <glm/glm.hpp>

#include "Scene/Core/Behaviour.hpp"
#include "Scene/Components/Camera.hpp"

class CameraController : Behaviour {
private:
	float move_speed{2.5f};
	float rotate_speed{0.1f};
	float mouse_sensitivity{0.1f};

	glm::vec2 last_mouse_pos{0.0f};
	bool      first_mouse{true};

	Camera* camera{nullptr};

public:
	CameraController(std::string name = "CameraController");

	CameraController(const CameraController&) = delete;
	CameraController& operator=(const CameraController&) = delete;

	CameraController(CameraController&&) noexcept = default;
	CameraController& operator=(CameraController&&) noexcept = default;

	~CameraController() override = default;

	std::type_index getType() override;

	void start() override;
	void update(float dt) override;
};
