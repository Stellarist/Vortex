module;

#include <SDL3/SDL.h>

export module Editor.Window;

export import Core;

export namespace Vortex {

class Window {
private:
	SDL_Window* window{};
	SDL_Event   event{};

	uint32 width{};
	uint32 height{};
	bool   should_close{};

	std::vector<std::function<void()>> event_callbacks;

public:
	Window(std::string_view title, int width, int height);
	~Window();

	void pollEvent();
	void hook(std::function<void()> callback);

	uint32 getWidth() const;
	uint32 getHeight() const;

	void setWidth(uint32 width);
	void setHeight(uint32 height);

	bool shouldClose() const;

	SDL_Window* get() const;
	SDL_Event*  getEvent();
};

}        // namespace Vortex
