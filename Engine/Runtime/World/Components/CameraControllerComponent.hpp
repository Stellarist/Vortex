export module Runtime.World:CameraControllerComponent;

import Core;
import :Component;
import :CameraComponent;

export namespace Vortex {

enum class CameraMovement {
	Forward,
	Backward,
	Left,
	Right
};

class CameraControllerComponent : public Component {
private:
	float move_speed{5.0f};
	float mouse_sensitivity{0.1f};
	float scroll_sensitivity{0.01f};

	bool first_mouse{true};
	Vec2 last_mouse_pos{};

	PerspectiveCameraComponent* camera{};

public:
	CameraControllerComponent(std::string name);
	~CameraControllerComponent() override = default;

	void beginPlay() override;
	void tickComponent(float dt) override;
	void endPlay() override;

	void translate(CameraMovement movement, float dt);
	void rotate(const Vec2& mouse_pos);
	void scroll(float offset);
};

}        // namespace Vortex
