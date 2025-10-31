#pragma once

#include <functional>
#include <string_view>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Core/Input/Input.hpp"

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

static std::unordered_map<uint8_t, Mouse> mouse_map = {
    {SDL_BUTTON_LEFT, Mouse::LEFT},
    {SDL_BUTTON_MIDDLE, Mouse::MIDDLE},
    {SDL_BUTTON_RIGHT, Mouse::RIGHT},
};

static std::unordered_map<SDL_Keycode, Key> key_map = {
    {SDLK_W, Key::W},
    {SDLK_A, Key::A},
    {SDLK_S, Key::S},
    {SDLK_D, Key::D},
    {SDLK_ESCAPE, Key::ESCAPE},
    {SDLK_SPACE, Key::SPACE},
    {SDLK_RETURN, Key::ENTER},
};
