module Editor;

import :AssetImporter;

namespace Vortex {

Application::Application()
{
	Time::setMainClock(&clock);

	auto path = JsonParser::readJson(PathResolver::getConfigsDir() / "config.json")["scene"];
	auto scene = AssetImporter::loadScene(asset_manager, (PathResolver::getAssetsDir() / (path.get<std::string>())).string());

	window = std::make_unique<Window>("Vortex", 2560, 1440);

	world = std::make_unique<World>(asset_manager);
	world->setActiveScene(std::move(scene));

	renderer = std::make_unique<Renderer>(*window);
	renderer->setActiveWorld(*world);

	widget = std::make_unique<Widget>(*window, *renderer);

	window->hook([this]() {
		widget->pollEvent(*window->getEvent());
	});

	widget->hook([this]() {
		widget->drawSceneGraph(world.get(), clock.getDeltaTime());
	});

	renderer->hook([this](RHICommandList& command) {
		widget->drawFrame(renderer->getContext().getCommand());
	});
}

void Application::run()
{
	try {
		while (!window->shouldClose()) {
			clock.tick();

			float dt = clock.getDeltaTime();

			tickGui(dt);
			tickLogic(dt);
			tickRender(dt);
		}

		renderer->wait();

	} catch (const std::exception& e) {
		Logger::error(std::format("Error: {}", e.what()));
	}
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
	if (world) {
		world->setAssetManager(&asset_manager);
		renderer->setActiveWorld(*world);
	}
}

void Application::loadRenderer(std::unique_ptr<Renderer>&& new_renderer)
{
	renderer = std::move(new_renderer);
	if (world)
		renderer->setActiveWorld(*world);
}

}        // namespace Vortex
