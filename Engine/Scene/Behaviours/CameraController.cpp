#include "CameraController.hpp"

#include "Scene/Core/Node.hpp"
#include "Core/Input/InputHandler.hpp"

CameraController::CameraController(std::string name) :
    Behaviour(std::move(name))
{
}

std::type_index CameraController::getType()
{
	return typeid(CameraController);
}

void CameraController::start()
{
	setStarted(true);

	if (node && node->hasComponent<Camera>())
		camera = &node->getComponent<Camera>();
}

void CameraController::update(float dt)
{
	if (!camera)
		return;

	auto& handler = InputHandler::instance();

	if (handler.isKeyHeld(Key::W))
		translate(CameraMovement::FORWARD, dt);
	if (handler.isKeyHeld(Key::S))
		translate(CameraMovement::BACKWARD, dt);
	if (handler.isKeyHeld(Key::A))
		translate(CameraMovement::LEFT, dt);
	if (handler.isKeyHeld(Key::D))
		translate(CameraMovement::RIGHT, dt);

	if (handler.isMouseHeld(Mouse::RIGHT)) {
		if (!first_mouse)
			rotate(handler.getMousePos());
		else {
			last_mouse_pos = handler.getMousePos();
			first_mouse = false;
		}
	} else
		first_mouse = true;

	if (handler.getMouseScroll().y != 0.0f) {
		scroll(handler.getMouseScroll().y);
		handler.setMouseScroll(glm::vec2(0.0f));
	}
}

void CameraController::translate(CameraMovement movement, float dt)
{
	auto* persp_camera = dynamic_cast<PerspectiveCamera*>(camera);
	auto& transform = node->getTransform();

	glm::vec3 front = persp_camera->getFront();
	glm::vec3 right = persp_camera->getRight();

	switch (movement) {
	case CameraMovement::FORWARD:
		transform.translate(front * move_speed * dt);
		break;

	case CameraMovement::BACKWARD:
		transform.translate(-front * move_speed * dt);
		break;

	case CameraMovement::LEFT:
		transform.translate(-right * move_speed * dt);
		break;

	case CameraMovement::RIGHT:
		transform.translate(right * move_speed * dt);
		break;
	}
}

void CameraController::rotate(const glm::vec2& mouse_pos)
{
	auto delta = mouse_pos - last_mouse_pos;
	last_mouse_pos = mouse_pos;

	float yaw = delta.x * mouse_sensitivity;
	float pitch = delta.y * mouse_sensitivity;

	auto right = dynamic_cast<PerspectiveCamera*>(camera)->getRight();
	node->getTransform().rotate(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(-yaw));
	node->getTransform().rotate(right, glm::radians(-pitch));
}

void CameraController::scroll(float yoffset)
{
	auto* persp_camera = dynamic_cast<PerspectiveCamera*>(camera);
	float fov = persp_camera->getFov() - yoffset * scroll_sensitivity;
	fov = glm::clamp(fov, 0.1f, 1.57f);
	persp_camera->setFov(fov);
}
