#pragma once

#include <memory>

#include "Render/Renderer.hpp"
#include "Scene/World.hpp"
#include "Platform/Window.hpp"
#include "Platform/Widget.hpp"

class Application {
private:
	std::unique_ptr<World>    world;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<Window>   window;
	std::unique_ptr<Widget>   widget;

	float last_time{0.0f};
	float delta_time{0.0f};

public:
	Application();
	~Application() = default;

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	Application(Application&&) noexcept = default;
	Application& operator=(Application&&) noexcept = default;

	void run();
	void elapseTime();

	void tickGui();
	void tickLogic(float dt);
	void tickRender(float dt);

	void loadWorld(std::unique_ptr<World>&& world);
	void loadRenderer(std::unique_ptr<Renderer>&& renderer);
};
