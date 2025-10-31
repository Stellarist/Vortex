#pragma once

#include <unordered_map>

#include <glm/glm.hpp>

#include "Core/Input/Input.hpp"

class InputHandler {
private:
	InputHandler() = default;

	std::unordered_map<uint8_t, InputState> key_states;
	std::unordered_map<uint8_t, InputState> mouse_states;

	glm::vec2 mouse_pos{};
	glm::vec2 mouse_scroll{};

public:
	static InputHandler& instance();

	void onKeyInput(const KeyInput& input);
	void onMouseInput(const MouseInput& input);

	bool isKeyHeld(Key key) const;
	bool isMouseHeld(Mouse mouse) const;

	glm::vec2 getMousePos() const;
	void      setMousePos(const glm::vec2& pos);

	glm::vec2 getMouseScroll() const;
	void      setMouseScroll(const glm::vec2& scroll);
};
