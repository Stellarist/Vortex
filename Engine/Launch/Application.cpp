#include "Application.hpp"

#include <chrono>

#include "Utils/AssetImporter.hpp"

#include <Core/Log/Logger.hpp>

Application::Application()
{
	auto scene = AssetImporter::loadScene(ASSETS_DIR "dragon.gltf");

	window = std::make_unique<Window>("Vortex", 2560, 1440);

	world = std::make_unique<World>();
	world->setActiveScene(std::move(scene));

	renderer = std::make_unique<Renderer>(*window);
	renderer->setActiveWorld(*world);

	widget = std::make_unique<Widget>(*window, *renderer);

	window->hook([this]() {
		widget->pollEvent(*window->getEvent());
	});

	renderer->hook([this]() {
		widget->drawFrame(renderer->getCurrentFrame().getCurrentCommandBuffer());
	});

	widget->hook([this]() {
		widget->drawSceneGraph(world.get());
	});
}

void Application::run()
{
	while (!window->shouldClose()) {
		elapseTime();

		tickGui(delta_time);
		tickLogic(delta_time);
		tickRender(delta_time);
	}

	renderer->wait();
}

void Application::elapseTime()
{
	float current_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	delta_time = current_time - last_time;
	last_time = current_time;
}

void Application::tickGui(float dt)
{
	window->pollEvent();

	widget->newFrame();
}

void Application::tickLogic(float dt)
{
	if (world)
		world->tick(dt);
}

void Application::tickRender(float dt)
{
	if (renderer)
		renderer->tick(dt);
}

void Application::loadWorld(std::unique_ptr<World>&& new_world)
{
	world = std::move(new_world);
	if (world)
		renderer->setActiveWorld(*world);
}

void Application::loadRenderer(std::unique_ptr<Renderer>&& new_renderer)
{
	renderer = std::move(new_renderer);
	if (world)
		renderer->setActiveWorld(*world);
}
