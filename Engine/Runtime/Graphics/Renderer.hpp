export module Runtime.Graphics:Renderer;

import Core;
import Editor.Window;
import Runtime.World;
import :RHI.Command;
import :RHI.Context;
import :RHI.Pipeline;
import :Render.Scene;

export namespace Vortex {

enum class RenderPathType : uint8 {
	Forward,
	Deferred,
};

class Renderer {
	RHIExtent extent{};

	std::unique_ptr<RHIContext> context;
	std::unique_ptr<RenderScene> render_scene;

	RHIRef<RHIInputLayout> scene_input_layout;
	RHIRef<RHIShader> scene_vertex_shader;
	RHIRef<RHIShader> scene_pixel_shader;
	RHIRef<RHIGraphicsPipeline> scene_pipeline;
	RHIRef<RHIFramebuffer> framebuffer;
	RHIRef<RHITexture> depth_buffer;
	RHIRef<RHITextureView> depth_view;

	World* active_world{};

	RenderPathType path_type{RenderPathType::Forward};

	std::vector<std::function<void(RHICommandList&)>> render_callbacks;

	void createScenePipeline(RHIFramebuffer& framebuffer);
	void drawScene(RHICommandList& command);

public:
	Renderer(Window& window);
	~Renderer();

	void render();
	void wait();
	void draw(RHICommandList& command);
	void hook(std::function<void(RHICommandList&)> callback);

	void tick(float dt);

	void setActiveWorld(World& world);
	void setRenderPath(RenderPathType new_path_type);

	World* getActiveWorld() const { return active_world; }

	RHIContext& getContext() const { return *context; }
};

}        // namespace Vortex
