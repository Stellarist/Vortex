module Editor;

namespace Vortex {

CameraControllerComponent::CameraControllerComponent(std::string component_name) :
    Component(std::move(component_name))
{
	setTickEnabled(true);
}

void CameraControllerComponent::beginPlay()
{
	auto* owner = getOwner();
	camera = owner && owner->hasComponent<CameraComponent>() ? dynamic_cast<PerspectiveCameraComponent*>(&owner->getComponent<CameraComponent>()) : nullptr;
}

void CameraControllerComponent::tickComponent(float dt)
{
	auto* owner = getOwner();
	camera = owner && owner->hasComponent<CameraComponent>() ? dynamic_cast<PerspectiveCameraComponent*>(&owner->getComponent<CameraComponent>()) : nullptr;
	if (!camera)
		return;

	auto& handler = InputHandler::instance();
	if (handler.isKeyHeld(Input::Key::W))
		translate(CameraMovement::Forward, dt);
	if (handler.isKeyHeld(Input::Key::S))
		translate(CameraMovement::Backward, dt);
	if (handler.isKeyHeld(Input::Key::A))
		translate(CameraMovement::Left, dt);
	if (handler.isKeyHeld(Input::Key::D))
		translate(CameraMovement::Right, dt);

	if (handler.isMouseHeld(Input::Mouse::Right)) {
		if (first_mouse) {
			last_mouse_pos = handler.getMousePos();
			first_mouse = false;
		} else {
			rotate(handler.getMousePos());
		}
	} else {
		first_mouse = true;
	}

	if (handler.getMouseScroll().y != 0.0f) {
		scroll(handler.getMouseScroll().y);
		handler.setMouseScroll(Vec2(0.0f));
	}
}

void CameraControllerComponent::endPlay()
{
	camera = nullptr;
	first_mouse = true;
}

void CameraControllerComponent::translate(CameraMovement movement, float dt)
{
	auto* root_component = getOwner() ? getOwner()->getRootComponent() : nullptr;
	if (!camera || !root_component)
		return;

	auto& transform = root_component->getTransform();
	const Vec3 front = camera->getFront();
	const Vec3 right = camera->getRight();
	switch (movement) {
	case CameraMovement::Forward:
		transform.translate(front * move_speed * dt);
		break;
	case CameraMovement::Backward:
		transform.translate(-front * move_speed * dt);
		break;
	case CameraMovement::Left:
		transform.translate(-right * move_speed * dt);
		break;
	case CameraMovement::Right:
		transform.translate(right * move_speed * dt);
		break;
	}
}

void CameraControllerComponent::rotate(const Vec2& mouse_pos)
{
	auto* root_component = getOwner() ? getOwner()->getRootComponent() : nullptr;
	if (!root_component)
		return;

	const auto delta = mouse_pos - last_mouse_pos;
	last_mouse_pos = mouse_pos;
	const float yaw = Math::radians(-delta.x * mouse_sensitivity);
	const float pitch = Math::radians(-delta.y * mouse_sensitivity);

	auto& transform = root_component->getTransform();
	const auto rotation = transform.getRotation();
	const Quat yaw_quat = Math::angleAxis(yaw, Vec3(0.0f, 1.0f, 0.0f));
	const Quat pitch_quat = Math::angleAxis(pitch, Vec3(1.0f, 0.0f, 0.0f));

	transform.setRotation(yaw_quat * rotation * pitch_quat);
}

void CameraControllerComponent::scroll(float offset)
{
	if (!camera)
		return;
	const float fov = Math::clamp(camera->getFov() - offset * scroll_sensitivity, 0.1f, 1.57f);
	camera->setFov(fov);
}

}        // namespace Vortex
