#pragma once

#include "RHI/RHICommand.hpp"
#include "RHI/RHIPipeline.hpp"
#include "RHI/RHIResources.hpp"
#include "Runtime/World/World.hpp"

class Window;
class RenderScene;

enum class RenderPathType : uint8_t {
	Forward,
	Deferred,
};

class Renderer {
	RHIExtent extent{};

	std::unique_ptr<RHIContext>  context;
	std::unique_ptr<RenderScene> render_scene;

	std::unique_ptr<RHIInputLayout>      scene_input_layout;
	std::unique_ptr<RHIShader>           scene_vertex_shader;
	std::unique_ptr<RHIShader>           scene_fragment_shader;
	std::unique_ptr<RHIGraphicsPipeline> scene_pipeline;
	std::unique_ptr<RHIFrameBuffer>      frame_buffer;
	std::unique_ptr<RHITexture>          depth_buffer;

	World* active_world{};

	RenderPathType path_type{RenderPathType::Forward};

	std::vector<std::function<void(RHICommandList&)>> render_callbacks;

	void createScenePipeline(RHIFrameBuffer& framebuffer);
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
