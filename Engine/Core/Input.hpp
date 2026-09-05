export module Core:Input;

import std;
import :Types;
import :Math;

export namespace Vortex {

export namespace Input {
enum class Key : uint8 {
	W,
	A,
	S,
	D,
	Escape,
	Space,
	Enter,
	Count,
};

enum class Mouse : uint8 {
	Left,
	Right,
	Middle,
	Count,
};

enum class State : uint8 {
	Pressed,
	Released,
};

}        // namespace Input

class InputHandler {
private:
	InputHandler() = default;

	static constexpr size_t key_count{static_cast<size_t>(Input::Key::Count)};
	static constexpr size_t mouse_count{static_cast<size_t>(Input::Mouse::Count)};

	std::array<bool, key_count> key_held{};
	std::array<bool, key_count> key_pressed{};
	std::array<bool, key_count> key_released{};
	std::array<bool, mouse_count> mouse_held{};
	std::array<bool, mouse_count> mouse_pressed{};
	std::array<bool, mouse_count> mouse_released{};

	Vec2 mouse_pos{};
	Vec2 mouse_scroll{};

public:
	static InputHandler& instance();

	void beginFrame();
	void reset();

	void onKey(Input::Key key, Input::State state);
	void onMouse(Input::Mouse mouse, const Vec2& pos, Input::State state);

	bool isKeyHeld(Input::Key key) const;
	bool wasKeyPressed(Input::Key key) const;
	bool wasKeyReleased(Input::Key key) const;
	bool isMouseHeld(Input::Mouse mouse) const;
	bool wasMousePressed(Input::Mouse mouse) const;
	bool wasMouseReleased(Input::Mouse mouse) const;

	Vec2 getMousePos() const;
	void setMousePos(const Vec2& pos);

	Vec2 getMouseScroll() const;
	void setMouseScroll(const Vec2& scroll);
};

}        // namespace Vortex
