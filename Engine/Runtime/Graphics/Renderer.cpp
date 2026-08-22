module Runtime.Graphics;

namespace Vortex {

Renderer::Renderer(Window& window)
{
	context = std::make_unique<VulkanContext>(window);
}

Renderer::~Renderer()
{
	wait();
}

void Renderer::render()
{
	context->beginFrame();
	auto& command = context->getCommand();
	command.clearTexture(&context->getBackbuffer(), RHIColor{0.1f, 0.1f, 0.1f, 1.0f});
	context->endFrame();
}

void Renderer::wait()
{
	context->getDevice().waitIdle();
}

void Renderer::draw(RHICommandList& command)
{
	if (path_type == RenderPathType::Forward || path_type == RenderPathType::Deferred) {
		drawScene(command);
		for (auto& callback : render_callbacks)
			callback(command);
	}
}

void Renderer::hook(std::function<void(RHICommandList&)> callback)
{
	render_callbacks.push_back(std::move(callback));
}

void Renderer::tick(float dt)
{
	if (!active_world)
		return;

	if (!render_scene)
		render_scene = std::make_unique<RenderScene>(*context, *active_world);
	render_scene->update(dt);

	context->beginFrame();
	auto& command = context->getCommand();
	command.clearTexture(&context->getBackbuffer(), RHIColor{0.1f, 0.1f, 0.1f, 1.0f});
	draw(command);
	context->endFrame();
}

void Renderer::setActiveWorld(World& world)
{
	active_world = &world;
	render_scene.reset();
	scene_pipeline.reset();
}

void Renderer::setRenderPath(RenderPathType new_path_type)
{
	if (path_type == new_path_type)
		return;

	context->getDevice().waitIdle();
	path_type = new_path_type;
	scene_vertex_shader.reset();
	scene_pixel_shader.reset();
	scene_pipeline.reset();
}

void Renderer::createScenePipeline(RHIFramebuffer& framebuffer)
{
	if (!scene_input_layout) {
		RHIInputLayoutDesc desc{};
		desc.addBindingDesc(RenderVertex::binding())
		    .setAttributeDescs(RenderVertex::attributes());

		scene_input_layout = context->getDevice().createInputLayout(desc);
	}

	if (!scene_vertex_shader || !scene_pixel_shader) {
		auto shader_name = path_type == RenderPathType::Forward ? std::string{"Forward"} : std::string{"Deferred"};
		auto shader_data = FileSystem::readBinaryFile(PathResolver::getShadersDir() / (shader_name + ".spv"));

		RHIShaderDesc vs_desc{};
		vs_desc.setType(RHIShaderType::Vertex)
		    .setEntryPoint("vertexMain")
		    .setShaderName(shader_name);
		scene_vertex_shader = context->getDevice().createShader(vs_desc, shader_data);

		RHIShaderDesc fs_desc{};
		fs_desc.setType(RHIShaderType::Pixel)
		    .setEntryPoint("pixelMain")
		    .setShaderName(shader_name);
		scene_pixel_shader = context->getDevice().createShader(fs_desc, shader_data);
	}

	RHIGraphicsPipelineDesc pipeline_desc{};
	pipeline_desc.setInputLayout(*scene_input_layout)
	    .setVertexShader(*scene_vertex_shader)
	    .setPixelShader(*scene_pixel_shader)
	    .addBindingLayout(*render_scene->getSceneLayout())
	    .addBindingLayout(*render_scene->getMaterialLayout())
	    .addBindingLayout(*render_scene->getObjectLayout());

	RHIRasterState raster_state{};
	raster_state.setCullMode(RHICullMode::None);
	pipeline_desc.setRasterState(raster_state);

	RHIDepthStencilState depth_state{};
	depth_state.setDepthTestEnable(true)
	    .setDepthWriteEnable(true)
	    .setDepthCompareOp(RHICompareOp::LessOrEqual);
	pipeline_desc.setDepthStencilState(depth_state);

	scene_pipeline = context->getDevice().createGraphicsPipeline(pipeline_desc, framebuffer);
}

void Renderer::drawScene(RHICommandList& command)
{
	if (!render_scene)
		return;

	auto extent = context->getExtent();
	if (!depth_buffer || depth_buffer->getDesc().width != extent.width || depth_buffer->getDesc().height != extent.height) {
		RHITextureDesc depth_desc{};
		depth_desc.setWidth(extent.width)
		    .setHeight(extent.height)
		    .setFormat(RHIFormat::D32_FLOAT)
		    .setUsage(RHITextureUsage::DepthStencil | RHITextureUsage::CopyDest);
		depth_buffer = context->getDevice().createTexture(depth_desc);
		depth_view = context->getDevice().createTextureView(
		    RHITextureViewDesc{}
		        .setTexture(depth_buffer.get())
		        .setType(RHITextureViewType::DepthStencil));
		scene_pipeline.reset();
	}

	RHIFramebufferDesc framebuffer_desc{};
	framebuffer_desc.setWidth(extent.width)
	    .setHeight(extent.height)
	    .addColorAttachment(RHIFramebufferAttachment{}
	            .setTextureView(&context->getBackbufferView())
	            .setFormat(context->getFormat()))
	    .setDepthAttachment(depth_view.get());
	framebuffer = context->getDevice().createFramebuffer(framebuffer_desc);

	if (!scene_pipeline)
		createScenePipeline(*framebuffer);

	command.clearDepthTexture(depth_buffer.get(), true, 1.0f, false, 0);

	RHIViewportState viewport_state{};
	viewport_state.addViewport(RHIViewport(static_cast<float>(extent.width), static_cast<float>(extent.height)))
	    .addScissor(RHIRect(static_cast<int>(extent.width), static_cast<int>(extent.height)));

	RHIGraphicsState graphics_state{};
	graphics_state.setFramebuffer(framebuffer.get())
	    .setPipeline(scene_pipeline.get())
	    .setViewport(viewport_state);

	render_scene->draw(command, graphics_state);
	command.clear();
}

}        // namespace Vortex
