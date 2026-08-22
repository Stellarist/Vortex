export module Core.Input;

import std;
import Core.Types;
import Core.Math;

export namespace Vortex {

class Input {
public:
	enum class Key : uint8 {
		W,
		A,
		S,
		D,
		Escape,
		Space,
		Enter,
	};

	enum class Mouse : uint8 {
		Left,
		Right,
		Middle,
	};

	enum class Type : uint8 {
		Keyboard,
		Mouse,
	};

	enum class State : uint8 {
		Undefined,
		Pressed,
		Released,
	};

protected:
	Type  type;
	State state;

public:
	Input(Type type, State state);
	virtual ~Input() = default;

	Type  getType() const;
	State getState() const;
};


class KeyInput : public Input {
private:
	Key key;

public:
	KeyInput(Key key, State state);

	Key getKey() const;
};


class MouseInput : public Input {
private:
	Mouse mouse;

	Vec2 pos;

public:
	MouseInput(Mouse mouse, const Vec2& pos, State state);

	Mouse getMouse() const;
	Vec2  getPosition() const;
};


class InputHandler {
private:
	InputHandler() = default;

	std::unordered_map<uint8, Input::State> key_states;
	std::unordered_map<uint8, Input::State> mouse_states;

	Vec2 mouse_pos{};
	Vec2 mouse_scroll{};

public:
	static InputHandler& instance();

	void onKeyInput(const KeyInput& input);
	void onMouseInput(const MouseInput& input);

	bool isKeyHeld(Input::Key key) const;
	bool isMouseHeld(Input::Mouse mouse) const;

	Vec2 getMousePos() const;
	void setMousePos(const Vec2& pos);

	Vec2 getMouseScroll() const;
	void setMouseScroll(const Vec2& scroll);
};

}        // namespace Vortex
