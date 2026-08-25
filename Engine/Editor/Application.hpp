export module Editor:Application;

import Core;
import Runtime.World;
import Runtime.Graphics;
import Editor.Window;
import :Widget;

export namespace Vortex {

class Application {
private:
	AssetManager asset_manager;

	std::unique_ptr<World>    world;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<Window>   window;
	std::unique_ptr<Widget>   widget;

	Clock clock;

public:
	Application();

	void run();

	void tickGui(float dt);
	void tickLogic(float dt);
	void tickRender(float dt);

	void loadWorld(std::unique_ptr<World>&& world);
	void loadRenderer(std::unique_ptr<Renderer>&& renderer);
};

}        // namespace Vortex
