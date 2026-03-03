#pragma once

#include <functional>
#include <string_view>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Runtime/Core/Input.hpp"

class Window {
private:
	SDL_Window* window{};
	SDL_Event   event{};

	uint32_t width{};
	uint32_t height{};
	bool     should_close{};

	std::vector<std::function<void()>> event_callbacks;

public:
	Window(std::string_view title, int width, int height);
	~Window();

	void pollEvent();
	void hook(std::function<void()> callback);

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	void setWidth(uint32_t width);
	void setHeight(uint32_t height);

	bool shouldClose() const;

	SDL_Window* get() const;
	SDL_Event*  getEvent();
};

static std::unordered_map<uint8_t, Input::Mouse> mouse_map = {
    {SDL_BUTTON_LEFT, Input::Mouse::Left},
    {SDL_BUTTON_MIDDLE, Input::Mouse::Middle},
    {SDL_BUTTON_RIGHT, Input::Mouse::Right},
};

static std::unordered_map<SDL_Keycode, Input::Key> key_map = {
    {SDLK_W, Input::Key::W},
    {SDLK_A, Input::Key::A},
    {SDLK_S, Input::Key::S},
    {SDLK_D, Input::Key::D},
    {SDLK_ESCAPE, Input::Key::Escape},
    {SDLK_SPACE, Input::Key::Space},
    {SDLK_RETURN, Input::Key::Enter},
};
