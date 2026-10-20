import Editor;
import Runtime.Vulkan;
import Runtime.D3D12;

int main(int argc, char** argv)
{
	try {
		Vortex::File::initialize(std::filesystem::path(argv[0]));
		Vortex::LOG("Vortex starting from '{}'", Vortex::File::executableDir().string());

		auto config = Vortex::Config::load(
		    Vortex::File::executableDir() / "config.toml");
		Vortex::uint64 frame_limit{};
		for (int i = 1; i < argc; ++i) {
			const std::string_view option = argv[i];
			Vortex::CHECK(i + 1 < argc, "Expected a value after '{}'", option);
			const std::string_view value = argv[++i];
			if (option == "--rhi") {
				Vortex::CHECK(value == "vulkan" || value == "d3d12", "--rhi must be vulkan or d3d12");
				config.api = value == "d3d12" ? Vortex::RHIAPI::D3D12 : Vortex::RHIAPI::Vulkan;
			} else if (option == "--frames") {
				const auto result = std::from_chars(value.data(), value.data() + value.size(), frame_limit);
				Vortex::CHECK(result.ec == std::errc{} && result.ptr == value.data() + value.size() && frame_limit > 0, "--frames requires a positive integer");
			} else
				Vortex::ERROR("Unknown option '{}'", option);
		}
		auto world = Vortex::loadGltfWorld(
		    (Vortex::File::assetsDir() / config.scene).string());

		Vortex::Window window("Vortex", 2560, 1440, config.api == Vortex::RHIAPI::Vulkan);
		world->beginPlay();

		std::unique_ptr<Vortex::RHIContext> context;
		if (config.api == Vortex::RHIAPI::D3D12)
			context = std::make_unique<Vortex::D3D12Context>(window);
		else
			context = std::make_unique<Vortex::VulkanContext>(window);
		Vortex::DeferredRenderer renderer(*context);

		Vortex::Widget widget(window, renderer);
		Vortex::Clock clock;

		window.hook([&window, &widget]() {
			widget.pollEvent(*window.getEvent());
		});

		widget.hook([&widget, &world]() {
			widget.drawSceneGraph(world.get());
		});

		while (!window.shouldClose() && (!frame_limit || renderer.getStats().frame_index < frame_limit)) {
			Vortex::InputHandler::instance().beginFrame();
			window.pollEvent();
			if (window.shouldClose())
				break;

			const bool frame_ready = renderer.beginFrame();
			// Include events received during acquire/fence waits in this frame's camera.
			if (frame_ready)
				window.pollEvent();
			clock.tick();
			const float dt = clock.getDeltaTime();

			widget.newFrame(dt);
			world->tick(dt);
			if (frame_ready)
				renderer.render(Vortex::RenderFrameRequest{
				    .world = world.get(),
				    .overlay = [&widget](Vortex::RHICommandList& command) {
					    widget.drawFrame(command);
				    },
				});
		}

		renderer.waitIdle();
		if (auto* vulkan = dynamic_cast<Vortex::VulkanContext*>(context.get()))
			Vortex::CHECK(vulkan->getValidationErrorCount() == 0, "Vulkan validation reported errors");
		if (auto* d3d12 = dynamic_cast<Vortex::D3D12Context*>(context.get()))
			Vortex::CHECK(d3d12->getValidationErrorCount() == 0, "D3D12 validation reported errors");
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
