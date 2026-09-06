module Runtime.Render;

import Runtime.RDG;
import Runtime.Vulkan;

namespace Vortex {

static RenderViewDesc extractRenderView(const World& world)
{
	RenderViewDesc result{};
	if (const auto* camera = world.getActiveCamera()) {
		result.camera_id = camera->getUid();
		result.view = camera->getView();
		result.projection = camera->getProjection();
		result.position = camera->getWorldPosition();
		result.has_camera = true;
	}
	return result;
}

Renderer::Renderer(Window& window, RenderSettings initial_settings) :
    settings(std::move(initial_settings))
{
	context = std::make_unique<VulkanContext>(window);
	render_pipeline = std::make_unique<RenderPipeline>(context->getDevice());
	backbuffer_extent = context->getExtent();
	LOG("Renderer initialized ({}x{})", backbuffer_extent.width, backbuffer_extent.height);
}

Renderer::~Renderer()
{
	wait();
	LOG(Debug, "Renderer shut down");
}

bool Renderer::updateBackbufferExtent()
{
	const auto extent = context->getExtent();
	if (extent == backbuffer_extent)
		return false;
	LOG("Renderer backbuffer resized from {}x{} to {}x{}",
	    backbuffer_extent.width,
	    backbuffer_extent.height,
	    extent.width,
	    extent.height);
	backbuffer_extent = extent;
	return true;
}

void Renderer::render()
{
	stats.frame_rendered = false;
	if (!context->beginFrame())
		return;
	updateBackbufferExtent();
	auto& command = context->getCommand();
	command.transitionTexture(
	    &context->getBackbuffer(), context->getBackbuffer().getState(), CopyDest);
	command.clearTexture(
	    &context->getBackbuffer(), RHIColor{0.1f, 0.1f, 0.1f, 1.0f});
	context->endFrame();
	stats.frame_rendered = true;
	++stats.frame_index;
}

void Renderer::wait()
{
	context->getDevice().waitIdle();
}

void Renderer::draw(RHICommandList& command)
{
	if (!render_scene)
		return;

	auto frame_settings = settings;
	if (!std::isfinite(frame_settings.shadow_bias) || frame_settings.shadow_bias < 0.0f ||
	    frame_settings.shadow_bias > 0.05f) {
		last_setting_error = "Invalid shadow bias; using the default for this frame";
		frame_settings.shadow_bias = 0.0015f;
	}
	RDGBuilder graph;
	auto backbuffer = graph.registerExternalTexture(
	    "Backbuffer", context->getBackbuffer(), context->getBackbuffer().getState(), CopySource);
	const auto frame = view_builder.build(
	    *render_scene, active_view, frame_settings.frustum_culling);
	render_pipeline->build(
	    graph, backbuffer, frame, frame_settings, context->getExtent());
	graph.compile();
	stats.rdg_pass_count = static_cast<uint32>(std::ranges::count_if(
	    graph.getPasses(), [](const RDGPassNode& pass) { return !pass.state.culled; }));
	graph.execute(context->getDevice(), command);
	const auto& frame_stats = frame.getStats();
	stats.opaque_draw_count = static_cast<uint32>(frame_stats.opaque_draw_count);
	stats.masked_draw_count = static_cast<uint32>(frame_stats.masked_draw_count);
	stats.transparent_draw_count = static_cast<uint32>(frame_stats.transparent_draw_count);
	stats.draw_count = stats.opaque_draw_count + stats.masked_draw_count +
	    stats.transparent_draw_count;
	stats.shadow_draw_count = static_cast<uint32>(frame_stats.shadow_draw_count);
	stats.culled_draw_count = static_cast<uint32>(frame_stats.culled_draw_count);

	for (auto& callback : render_callbacks)
		callback(command);
}

void Renderer::hook(std::function<void(RHICommandList&)> callback)
{
	render_callbacks.push_back(std::move(callback));
}

void Renderer::tick(float)
{
	stats.frame_rendered = false;
	if (!active_world)
		return;

	if (!render_scene)
		render_scene = std::make_unique<RenderScene>(*context);
	render_scene->update(*active_world);
	active_view = extractRenderView(*active_world);

	if (!context->beginFrame())
		return;
	updateBackbufferExtent();
	auto& command = context->getCommand();
	draw(command);
	context->endFrame();
	stats.frame_rendered = true;
	++stats.frame_index;
}

void Renderer::setActiveWorld(World& world)
{
	active_world = &world;
	active_view = {};
	render_scene.reset();
	render_pipeline->resetSceneState();
	stats = {};
	LOG("Renderer activated world '{}'", world.getName());
}

bool Renderer::setRenderPath(RenderPathType new_path_type)
{
	if (new_path_type != RenderPathType::Forward &&
	    new_path_type != RenderPathType::Deferred) {
		last_setting_error = "Unknown render path";
		LOG(Warn, "Rejected renderer setting: {}", last_setting_error);
		return false;
	}
	if (settings.render_path == new_path_type)
		return true;

	context->getDevice().waitIdle();
	settings.render_path = new_path_type;
	last_setting_error.clear();
	LOG("Renderer path changed to {}",
	    settings.render_path == RenderPathType::Forward ? "forward" : "deferred");
	return true;
}

}        // namespace Vortex
