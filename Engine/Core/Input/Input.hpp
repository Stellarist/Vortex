#pragma once

#include <glm/glm.hpp>

enum class Key : uint8_t;
enum class Mouse : uint8_t;

enum class InputType : uint8_t;
enum class InputState : uint8_t;

class Input {
protected:
	InputType  type;
	InputState state;

public:
	Input(InputType type, InputState state);
	virtual ~Input() = default;

	InputType  getType() const;
	InputState getState() const;
};

class KeyInput : public Input {
private:
	Key key;

public:
	KeyInput(Key key, InputState state);

	Key getKey() const;
};

class MouseInput : public Input {
private:
	Mouse mouse;

	glm::vec2 pos;

public:
	MouseInput(Mouse mouse, const glm::vec2& pos, InputState state);

	Mouse     getMouse() const;
	glm::vec2 getPosition() const;
};

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

enum class InputType : uint8_t {
	Keyboard,
	Mouse,
};

enum class InputState : uint8_t {
	Undefined,
	Pressed,
	Released,
};
