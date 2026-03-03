#include "Input.hpp"

Input::Input(Input::Type type, Input::State state) :
    type(type),
    state(state)
{}

Input::Type Input::getType() const
{
	return type;
}

Input::State Input::getState() const
{
	return state;
}

KeyInput::KeyInput(Key key, Input::State state) :
    Input(Input::Type::Keyboard, state),
    key(key)
{}

Input::Key KeyInput::getKey() const
{
	return key;
}

MouseInput::MouseInput(Mouse mouse, const glm::vec2& pos, Input::State state) :
    Input(Input::Type::Mouse, state),
    mouse(mouse),
    pos(pos)
{}

Input::Mouse MouseInput::getMouse() const
{
	return mouse;
}

glm::vec2 MouseInput::getPosition() const
{
	return pos;
}

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

bool InputHandler::isKeyHeld(Input::Key key) const
{
	auto it = key_states.find(static_cast<uint8_t>(key));
	return it != key_states.end() && it->second == Input::State::Pressed;
}

bool InputHandler::isMouseHeld(Input::Mouse mouse) const
{
	auto it = mouse_states.find(static_cast<uint8_t>(mouse));
	return it != mouse_states.end() && it->second == Input::State::Pressed;
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
