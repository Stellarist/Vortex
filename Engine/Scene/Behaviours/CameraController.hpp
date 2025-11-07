#pragma once

#include <glm/glm.hpp>

#include "Scene/Core/Behaviour.hpp"
#include "Scene/Components/Camera.hpp"

enum class CameraMovement;

class CameraController : public Behaviour {
private:
	float move_speed{5.0f};
	float mouse_sensitivity{0.1f};
	float scroll_sensitivity{0.01f};

	bool enable_move{false};
	bool enable_rotation{false};
	bool enable_scroll{false};
	bool first_mouse{true};

	glm::vec2 last_mouse_pos{};
	glm::vec2 last_scroll_offset{};

	Camera* camera{};

public:
	CameraController(std::string name);
	~CameraController() override = default;

	std::type_index getType() override;

	void start() override;
	void update(float dt) override;

	void translate(CameraMovement movement, float dt);
	void rotate(const glm::vec2& mouse_pos);
	void scroll(float yoffset);
};

enum class CameraMovement {
	Forward,
	Backward,
	Left,
	Right
};
