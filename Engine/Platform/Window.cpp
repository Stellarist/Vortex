#include "Window.hpp"

#include <print>

Window::Window(std::string_view title, int width, int height) :
    width(width), height(height)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::println("SDL initialize failed: {}", SDL_GetError());
		throw std::runtime_error("SDL initialization failed");
	}

	window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_VULKAN);
	if (!window) {
		std::println("SDL window creation failed: {}", SDL_GetError());
		throw std::runtime_error("SDL window creation failed");
	}
}

Window::~Window()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Window::pollEvents()
{
	while (SDL_PollEvent(&event)) {
		for (auto& callback : event_callbacks)
			callback();

		switch (event.type) {
		case SDL_EventType::SDL_EVENT_KEY_DOWN:
			if (event.key.key == SDLK_ESCAPE)
				should_close = true;
			break;

		case SDL_EventType::SDL_EVENT_QUIT:
			should_close = true;
			break;
		}
	}
}

void Window::hook(std::function<void()> callback)
{
	event_callbacks.push_back(std::move(callback));
}

uint32_t Window::getWidth() const
{
	return width;
}

uint32_t Window::getHeight() const
{
	return height;
}

void Window::setWidth(uint32_t width)
{
	this->width = width;
}

void Window::setHeight(uint32_t height)
{
	this->height = height;
}

bool Window::shouldClose() const
{
	return should_close;
}

SDL_Window* Window::get() const
{
	return window;
}

SDL_Event* Window::getEvent()
{
	return &event;
}
