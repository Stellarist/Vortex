#include "Window.hpp"

#include "Core/Input/InputHandler.hpp"

Window::Window(std::string_view title, int width, int height) : width(width), height(height)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(SDL_GetError());

	window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_VULKAN);
	if (!window)
		throw std::runtime_error(SDL_GetError());
}

Window::~Window()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Window::pollEvent()
{
	while (SDL_PollEvent(&event)) {
		for (auto& callback : event_callbacks)
			callback();

		switch (event.type) {
		case SDL_EventType::SDL_EVENT_MOUSE_MOTION:
		{
			glm::vec2 pos{event.motion.x, event.motion.y};
			InputHandler::instance().setMousePos(pos);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			if (!mouse_map.contains(event.button.button))
				return;

			glm::vec2  pos{event.button.x, event.button.y};
			MouseInput mouse_input(mouse_map[event.button.button], pos, InputState::Pressed);
			InputHandler::instance().onMouseInput(mouse_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP:
		{
			if (!mouse_map.contains(event.button.button))
				return;

			glm::vec2  pos{event.button.x, event.button.y};
			MouseInput mouse_input(mouse_map[event.button.button], pos, InputState::Released);
			InputHandler::instance().onMouseInput(mouse_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
		{
			glm::vec2 scroll{static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y)};
			InputHandler::instance().setMouseScroll(scroll);
			break;
		}

		case SDL_EventType::SDL_EVENT_KEY_DOWN:
		{
			if (!key_map.contains(event.key.key))
				return;

			KeyInput key_input(key_map[event.key.key], InputState::Pressed);
			InputHandler::instance().onKeyInput(key_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_KEY_UP:
		{
			if (!key_map.contains(event.key.key))
				return;

			KeyInput key_input(key_map[event.key.key], InputState::Released);
			InputHandler::instance().onKeyInput(key_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_QUIT:
		{
			should_close = true;
			break;
		}
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
