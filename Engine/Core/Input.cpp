module Core;

namespace Vortex {

InputHandler& InputHandler::instance()
{
	static InputHandler handler;
	return handler;
}

void InputHandler::beginFrame()
{
	key_pressed.fill(false);
	key_released.fill(false);
	mouse_pressed.fill(false);
	mouse_released.fill(false);
	mouse_scroll = Vec2{};
}

void InputHandler::reset()
{
	key_held.fill(false);
	mouse_held.fill(false);
	beginFrame();
}

void InputHandler::onKey(Input::Key key, Input::State state)
{
	const auto index = static_cast<size_t>(key);
	if (index >= key_held.size())
		return;

	const bool was_held = key_held[index];
	const bool is_held = state == Input::State::Pressed;
	key_held[index] = is_held;
	if (is_held && !was_held)
		key_pressed[index] = true;
	else if (!is_held && was_held)
		key_released[index] = true;
}

void InputHandler::onMouse(Input::Mouse mouse, const Vec2& pos, Input::State state)
{
	const auto index = static_cast<size_t>(mouse);
	if (index >= mouse_held.size())
		return;

	const bool was_held = mouse_held[index];
	const bool is_held = state == Input::State::Pressed;
	mouse_held[index] = is_held;
	if (is_held && !was_held)
		mouse_pressed[index] = true;
	else if (!is_held && was_held)
		mouse_released[index] = true;
	setMousePos(pos);
}

bool InputHandler::isKeyHeld(Input::Key key) const
{
	const auto index = static_cast<size_t>(key);
	return index < key_held.size() && key_held[index];
}

bool InputHandler::wasKeyPressed(Input::Key key) const
{
	const auto index = static_cast<size_t>(key);
	return index < key_pressed.size() && key_pressed[index];
}

bool InputHandler::wasKeyReleased(Input::Key key) const
{
	const auto index = static_cast<size_t>(key);
	return index < key_released.size() && key_released[index];
}

bool InputHandler::isMouseHeld(Input::Mouse mouse) const
{
	const auto index = static_cast<size_t>(mouse);
	return index < mouse_held.size() && mouse_held[index];
}

bool InputHandler::wasMousePressed(Input::Mouse mouse) const
{
	const auto index = static_cast<size_t>(mouse);
	return index < mouse_pressed.size() && mouse_pressed[index];
}

bool InputHandler::wasMouseReleased(Input::Mouse mouse) const
{
	const auto index = static_cast<size_t>(mouse);
	return index < mouse_released.size() && mouse_released[index];
}

Vec2 InputHandler::getMousePos() const
{
	return mouse_pos;
}

void InputHandler::setMousePos(const Vec2& pos)
{
	mouse_pos = pos;
}

Vec2 InputHandler::getMouseScroll() const
{
	return mouse_scroll;
}

void InputHandler::setMouseScroll(const Vec2& scroll)
{
	mouse_scroll = scroll;
}

}        // namespace Vortex
