module;

#include <SDL3/SDL.h>

module Editor.Window;

namespace Vortex {

const std::unordered_map<uint8, Input::Mouse> mouse_map = {
    {SDL_BUTTON_LEFT, Input::Mouse::Left},
    {SDL_BUTTON_MIDDLE, Input::Mouse::Middle},
    {SDL_BUTTON_RIGHT, Input::Mouse::Right},
};

const std::unordered_map<SDL_Keycode, Input::Key> key_map = {
    {SDLK_W, Input::Key::W},
    {SDLK_A, Input::Key::A},
    {SDLK_S, Input::Key::S},
    {SDLK_D, Input::Key::D},
    {SDLK_ESCAPE, Input::Key::Escape},
    {SDLK_SPACE, Input::Key::Space},
    {SDLK_RETURN, Input::Key::Enter},
};

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
			Vec2 pos{event.motion.x, event.motion.y};
			InputHandler::instance().setMousePos(pos);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			if (!mouse_map.contains(event.button.button))
				return;

			Vec2 pos{event.button.x, event.button.y};
			MouseInput mouse_input(mouse_map.at(event.button.button), pos, Input::State::Pressed);
			InputHandler::instance().onMouseInput(mouse_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP:
		{
			if (!mouse_map.contains(event.button.button))
				return;

			Vec2 pos{event.button.x, event.button.y};
			MouseInput mouse_input(mouse_map.at(event.button.button), pos, Input::State::Released);
			InputHandler::instance().onMouseInput(mouse_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
		{
			Vec2 scroll{static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y)};
			InputHandler::instance().setMouseScroll(scroll);
			break;
		}

		case SDL_EventType::SDL_EVENT_KEY_DOWN:
		{
			if (!key_map.contains(event.key.key))
				return;

			KeyInput key_input(key_map.at(event.key.key), Input::State::Pressed);
			InputHandler::instance().onKeyInput(key_input);
			break;
		}

		case SDL_EventType::SDL_EVENT_KEY_UP:
		{
			if (!key_map.contains(event.key.key))
				return;

			KeyInput key_input(key_map.at(event.key.key), Input::State::Released);
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

uint32 Window::getWidth() const
{
	return width;
}

uint32 Window::getHeight() const
{
	return height;
}

void Window::setWidth(uint32 width)
{
	this->width = width;
}

void Window::setHeight(uint32 height)
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

}        // namespace Vortex
