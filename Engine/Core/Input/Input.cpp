#include "Input.hpp"

Input::Input(InputType type, InputState state) :
    type(type),
    state(state)
{}

InputType Input::getType() const
{
	return type;
}

InputState Input::getState() const
{
	return state;
}

KeyInput::KeyInput(Key key, InputState state) :
    Input(InputType::Keyboard, state),
    key(key)
{}

Key KeyInput::getKey() const
{
	return key;
}

MouseInput::MouseInput(Mouse mouse, const glm::vec2& pos, InputState state) :
    Input(InputType::Mouse, state),
    mouse(mouse),
    pos(pos)
{}

Mouse MouseInput::getMouse() const
{
	return mouse;
}

glm::vec2 MouseInput::getPosition() const
{
	return pos;
}
