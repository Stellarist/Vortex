module Runtime.Render;

import Runtime.RDG;

namespace Vortex {

// Own every CPU span borrowed by passes until graph recording completes.
// Path implementations share the frame; no frame data escapes render().
RenderFrame::RenderFrame(RenderPreparedView new_prepared, RenderSettings new_settings,
    RHITexture& target, RDGResourcePool& pool) :
    prepared(std::move(new_prepared)), settings(new_settings), stats(prepared.stats), graph(pool)
{
	CHECK(prepared.view.extent.width && prepared.view.extent.height,
	    "A RenderFrame requires a nonzero view extent");
	CHECK(prepared.view.extent.width == target.getDesc().width &&
	        prepared.view.extent.height == target.getDesc().height,
	    "Frame output extent must match the prepared view");

	view.constants = graph.createConstantBuffer("Frame.View", prepared.constants);
	// A zero-light view still binds a valid, zero-filled structured buffer.
	view.lights = graph.createBuffer("Frame.Lights", RHIBufferDesc{}.setSize(std::max(size_t{1}, prepared.lights.size()) * sizeof(RenderLightConstants)).setStride(sizeof(RenderLightConstants)).setUsage(RHIBufferUsage::StorageBuffer),
	    std::as_bytes(std::span{prepared.lights}));
	output = graph.registerExternalTexture("Frame.Output", target, target.getState(), TransferSource);
}

void RenderFrame::execute(RHIDevice& device, RHICommandList& command)
{
	CHECK(!executed, "A RenderFrame can only record once");
	executed = true;
	graph.addOutput(output);
	graph.compile();
	stats.rdg_pass_count = static_cast<uint32>(std::ranges::count_if(graph.getPasses(),
	    [](const RDGPassNode& pass) { return !pass.state.culled; }));
	graph.execute(device, command);
}

Renderer::Renderer(RHIContext& new_context, RenderSettings initial_settings) :
    context(&new_context), settings(initial_settings), resources(new_context.getDevice()), resource_pool(new_context.getDevice())
{}

Renderer::~Renderer()
{
	waitIdle();
}

bool Renderer::beginFrame()
{
	CHECK(!frame_active, "A renderer frame is already active");
	frame_active = context->beginFrame();
	// Preserve the previous result for Widget's timing sample on success.
	if (!frame_active)
		stats.frame_rendered = false;
	return frame_active;
}

void Renderer::render(const RenderFrameRequest& request)
{
	CHECK(Argument, request.world, "A frame request needs a world");
	CHECK(frame_active, "Rendering requires a successful beginFrame");
	stats.frame_rendered = false;
	const auto frame_settings = settings;

	resource_pool.collect();
	const auto extent = context->getExtent();
	auto& command = context->getCommand();
	// Extraction and asset uploads finish before publishing frame-owned draw data.
	// AssetHandle payloads are only read here, on this single rendering thread.
	RenderFrame frame([&] {
		const auto scene = collectRenderScene(*request.world);
		resources.synchronize(scene, command);
		return prepareRenderView(scene, resources,
		    collectRenderView(*request.world, extent), frame_settings);
	}(),
	    frame_settings, context->getBackbuffer(), resource_pool);
	buildGraph(frame);
	frame.execute(resources.getDevice(), command);

	if (request.overlay)
		request.overlay(command);
	context->endFrame();
	frame_active = false;

	const auto next_index = stats.frame_index + 1;
	stats = frame.stats;
	stats.frame_index = next_index;
	stats.frame_rendered = true;
	if (next_index == 1)
		LOG("Renderer first frame: {} draws, {} shadow draws, {} RDG passes",
		    stats.draw_count, stats.shadow_draw_count, stats.rdg_pass_count);
}

void Renderer::waitIdle()
{
	context->getDevice().waitIdle();
}

RHIContext& Renderer::getContext() const noexcept
{
	return *context;
}

RenderSettings& Renderer::getSettings() noexcept
{
	return settings;
}

const RenderStats& Renderer::getStats() const noexcept
{
	return stats;
}

DeferredRenderer::DeferredRenderer(RHIContext& context, RenderSettings settings) :
    Renderer(context, settings),
    shadow_pass(getResources()),
    gbuffer_pass(getResources()),
    lighting_pass(getResources()),
    transparent_pass(getResources())
{
	LOG("Renderer initialized with deferred path");
}

void DeferredRenderer::buildGraph(RenderFrame& frame)
{
	auto& graph = frame.graph;
	const auto& prepared = frame.prepared;
	const auto shadow = shadow_pass.build(graph, {
	                                                 .camera_position = prepared.view.position,
	                                                 .direction = prepared.shadow_direction,
	                                                 .enabled = frame.settings.directional_shadows,
	                                                 .draws = prepared.shadow,
	                                             });
	const auto gbuffer = gbuffer_pass.build(graph, {
	                                                   .extent = prepared.view.extent,
	                                                   .view = frame.view,
	                                                   .draws = prepared.opaque,
	                                               });
	const auto color = lighting_pass.build(graph, {
	                                                  .extent = prepared.view.extent,
	                                                  .view = frame.view,
	                                                  .gbuffer = gbuffer,
	                                                  .shadow = shadow,
	                                                  .output_format = frame.output->desc.format,
	                                              });
	transparent_pass.build(graph, {
	                                  .view = frame.view,
	                                  .color = color,
	                                  .depth = gbuffer.depth,
	                                  .shadow = shadow,
	                                  .draws = prepared.transparent,
	                              });
	present_pass.build(graph, {.color = color, .output = frame.output});
}

}        // namespace Vortex
