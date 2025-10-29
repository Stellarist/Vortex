#pragma once

#include <functional>
#include <string_view>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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

	void pollEvents();
	void hook(std::function<void()> callback);

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	void setWidth(uint32_t width);
	void setHeight(uint32_t height);

	bool shouldClose() const;

	SDL_Window* get() const;
	SDL_Event*  getEvent();
};
