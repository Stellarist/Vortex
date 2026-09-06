export module Runtime.Render:Renderer;

import Core;
export import Editor.Window;
import Runtime.World;
import Runtime.RHI;
import :Frame;
import :Scene;
import :Scene.ViewBuilder;
import :Pipeline;
import :Settings;

export namespace Vortex {

class Renderer {
	std::unique_ptr<RHIContext> context{};
	std::unique_ptr<RenderScene> render_scene{};
	std::unique_ptr<RenderPipeline> render_pipeline{};
	RenderViewBuilder view_builder{};

	World* active_world{};
	RenderViewDesc active_view{};

	RHIExtent backbuffer_extent{};
	RenderSettings settings{};
	RendererStats stats{};

	std::string last_setting_error{};

	std::vector<std::function<void(RHICommandList&)>> render_callbacks{};

	bool updateBackbufferExtent();

public:
	Renderer(Window& window, RenderSettings initial_settings = {});
	~Renderer();

	void render();
	void wait();
	void draw(RHICommandList& command);
	void hook(std::function<void(RHICommandList&)> callback);

	void tick(float dt);

	void setActiveWorld(World& world);
	bool setRenderPath(RenderPathType new_path_type);

	World* getActiveWorld() const { return active_world; }

	RHIContext& getContext() const { return *context; }
	RenderPathType getRenderPath() const noexcept { return settings.render_path; }
	RenderSettings& getSettings() noexcept { return settings; }
	const RenderSettings& getSettings() const noexcept { return settings; }
	const RendererStats& getStats() const noexcept { return stats; }
	const std::string& getLastSettingError() const noexcept { return last_setting_error; }
};

}        // namespace Vortex
