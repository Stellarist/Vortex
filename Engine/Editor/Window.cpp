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
	CHECK(SDL_Init(SDL_INIT_VIDEO), "Failed to initialize SDL: {}", SDL_GetError());

	window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

	CHECK(window, "Failed to create window '{}': {}", title, SDL_GetError());
	LOG("Created window '{}' ({}x{})", title, width, height);
}

Window::~Window()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
	LOG(Debug, "Destroyed application window");
}

void Window::pollEvent()
{
	InputHandler::instance().beginFrame();

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
				continue;
			const Vec2 pos{event.button.x, event.button.y};
			InputHandler::instance().onMouse(
			    mouse_map.at(event.button.button), pos, Input::State::Pressed);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP:
		{
			if (!mouse_map.contains(event.button.button))
				continue;
			const Vec2 pos{event.button.x, event.button.y};
			InputHandler::instance().onMouse(
			    mouse_map.at(event.button.button), pos, Input::State::Released);
			break;
		}

		case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
		{
			Vec2 scroll{static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y)};
			auto& input = InputHandler::instance();
			input.setMouseScroll(input.getMouseScroll() + scroll);
			break;
		}

		case SDL_EventType::SDL_EVENT_KEY_DOWN:
		{
			if (!key_map.contains(event.key.key))
				continue;
			InputHandler::instance().onKey(key_map.at(event.key.key), Input::State::Pressed);
			break;
		}

		case SDL_EventType::SDL_EVENT_KEY_UP:
		{
			if (!key_map.contains(event.key.key))
				continue;
			InputHandler::instance().onKey(key_map.at(event.key.key), Input::State::Released);
			break;
		}

		case SDL_EventType::SDL_EVENT_QUIT:
		{
			should_close = true;
			LOG(Debug, "Application window close requested");
			break;
		}

		case SDL_EventType::SDL_EVENT_WINDOW_FOCUS_LOST:
			InputHandler::instance().reset();
			LOG(Debug, "Application window lost focus; input state reset");
			break;

		case SDL_EventType::SDL_EVENT_WINDOW_RESIZED:
		case SDL_EventType::SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			width = static_cast<uint32>(std::max(event.window.data1, 0));
			height = static_cast<uint32>(std::max(event.window.data2, 0));
			break;

		case SDL_EventType::SDL_EVENT_WINDOW_MINIMIZED:
			if (!minimized)
				LOG(Debug, "Application window minimized");
			minimized = true;
			break;

		case SDL_EventType::SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EventType::SDL_EVENT_WINDOW_RESTORED:
			if (minimized)
				LOG(Debug, "Application window restored");
			minimized = false;
			break;
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

void Window::getPixelSize(uint32& pixel_width, uint32& pixel_height) const noexcept
{
	int queried_width{};
	int queried_height{};
	if (window && SDL_GetWindowSizeInPixels(window, &queried_width, &queried_height)) {
		pixel_width = static_cast<uint32>(std::max(queried_width, 0));
		pixel_height = static_cast<uint32>(std::max(queried_height, 0));
		return;
	}

	pixel_width = width;
	pixel_height = height;
}

bool Window::isMinimized() const noexcept
{
	return minimized;
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
