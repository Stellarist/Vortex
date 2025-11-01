#include "InputHandler.hpp"

InputHandler& InputHandler::instance()
{
	static InputHandler handler;
	return handler;
}

void InputHandler::onKeyInput(const KeyInput& input)
{
	key_states[static_cast<uint8_t>(input.getKey())] = input.getState();
}

void InputHandler::onMouseInput(const MouseInput& input)
{
	mouse_states[static_cast<uint8_t>(input.getMouse())] = input.getState();
	setMousePos(input.getPosition());
}

bool InputHandler::isKeyHeld(Key key) const
{
	auto it = key_states.find(static_cast<uint8_t>(key));
	return it != key_states.end() && it->second == InputState::Pressed;
}

bool InputHandler::isMouseHeld(Mouse mouse) const
{
	auto it = mouse_states.find(static_cast<uint8_t>(mouse));
	return it != mouse_states.end() && it->second == InputState::Pressed;
}

glm::vec2 InputHandler::getMousePos() const
{
	return mouse_pos;
}

void InputHandler::setMousePos(const glm::vec2& pos)
{
	mouse_pos = pos;
}

glm::vec2 InputHandler::getMouseScroll() const
{
	return mouse_scroll;
}

void InputHandler::setMouseScroll(const glm::vec2& scroll)
{
	mouse_scroll = scroll;
}
