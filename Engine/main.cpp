import Editor;

int main(int argc, char** argv)
{
	try {
		Vortex::File::initialize(std::filesystem::path(argv[0]));
		Vortex::LOG("Vortex starting from '{}'", Vortex::File::executableDir().string());

		auto config = Vortex::ApplicationConfig::load(
		    Vortex::File::configsDir() / "config.yaml");
		auto graphics_settings = Vortex::RenderSettings::load(
		    Vortex::File::configsDir() / "graphics.yaml");
		auto world = Vortex::loadGltfWorld(
		    (Vortex::File::assetsDir() / config.scene).string());

		Vortex::Window window("Vortex", 2560, 1440);
		world->beginPlay();

		Vortex::Renderer renderer(window, std::move(graphics_settings));
		if (!renderer.getLastSettingError().empty())
			Vortex::LOG(Vortex::Warn, renderer.getLastSettingError());
		renderer.setActiveWorld(*world);

		Vortex::Widget widget(window, renderer);
		Vortex::Clock clock;

		window.hook([&window, &widget]() {
			widget.pollEvent(*window.getEvent());
		});

		widget.hook([&clock, &widget, &world]() {
			widget.drawSceneGraph(world.get(), clock.getDeltaTime());
		});

		renderer.hook([&widget](Vortex::RHICommandList& command) {
			widget.drawFrame(command);
		});

		while (!window.shouldClose()) {
			clock.tick();
			const float dt = clock.getDeltaTime();

			window.pollEvent();
			widget.newFrame();
			world->tick(dt);
			renderer.tick(dt);
		}

		renderer.wait();
		world->endPlay();

		Vortex::LOG("Vortex shutdown complete");
		Vortex::flushLog();
		return 0;

	} catch (const std::exception& error) {
		Vortex::LOG(Vortex::Error, "Fatal error: {}", error.what());
		Vortex::flushLog();
		return 1;
	}
}
