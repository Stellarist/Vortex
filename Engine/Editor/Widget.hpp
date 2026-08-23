module;

#include <SDL3/SDL.h>

export module Editor.Widget;

import vulkan;

export import Editor.Window;
export import Runtime.World;
export import Runtime.Graphics;

export namespace Vortex {

class Widget {
private:
	vk::DescriptorPool descriptor_pool{};
	vk::Device         device{};

	std::vector<std::function<void()>> draw_callbacks{};

	Renderer* renderer{};
	Window*   window{};

public:
	Widget(Window& window, Renderer& renderer);
	~Widget();

	void newFrame();
	void drawFrame(RHICommandList& command);

	bool pollEvent(const SDL_Event& event);
	void hook(std::function<void()> callback);

	void drawSceneGraph(const World* world, float dt);
	void drawSceneActors(const Actor* actor);
	void drawSceneComponents(const Scene* scene);
	void drawAssets(const AssetManager* assets);
};

}        // namespace Vortex
