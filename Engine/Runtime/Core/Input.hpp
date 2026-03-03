#pragma once

#include <glm/glm.hpp>

class Input {
public:
	enum class Key : uint8_t {
		W,
		A,
		S,
		D,
		Escape,
		Space,
		Enter,
	};

	enum class Mouse : uint8_t {
		Left,
		Right,
		Middle,
	};

	enum class Type : uint8_t {
		Keyboard,
		Mouse,
	};

	enum class State : uint8_t {
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

	glm::vec2 pos;

public:
	MouseInput(Mouse mouse, const glm::vec2& pos, State state);

	Mouse     getMouse() const;
	glm::vec2 getPosition() const;
};

class InputHandler {
private:
	InputHandler() = default;

	std::unordered_map<uint8_t, Input::State> key_states;
	std::unordered_map<uint8_t, Input::State> mouse_states;

	glm::vec2 mouse_pos{};
	glm::vec2 mouse_scroll{};

public:
	static InputHandler& instance();

	void onKeyInput(const KeyInput& input);
	void onMouseInput(const MouseInput& input);

	bool isKeyHeld(Input::Key key) const;
	bool isMouseHeld(Input::Mouse mouse) const;

	glm::vec2 getMousePos() const;
	void      setMousePos(const glm::vec2& pos);

	glm::vec2 getMouseScroll() const;
	void      setMouseScroll(const glm::vec2& scroll);
};
