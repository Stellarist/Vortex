module;

#include <cstddef>

module Runtime.Render;

namespace Vortex {

static_assert(std::is_standard_layout_v<ShadowData>);
static_assert(offsetof(ShadowData, light_view_projection) == 0);
static_assert(offsetof(ShadowData, params) == 64);
static_assert(sizeof(ShadowData) == 80);

static RHIViewportState shadowViewport(const RHIFramebufferDesc& desc)
{
	RHIViewportState viewport{};
	viewport.addViewport(RHIViewport(static_cast<float>(desc.width), static_cast<float>(desc.height)))
	    .addScissor(RHIRect(static_cast<int>(desc.width), static_cast<int>(desc.height)));
	return viewport;
}

ShadowPass::ShadowPass(PipelineResources& pipeline_resources,
    const RenderFrame& frame, ShadowPassParams pass_parameters) :
    RDGPass(RDGPassDesc{.name = "Shadow.Directional", .type = RDGPassType::Raster}),
    resources(&pipeline_resources),
    frame(&frame),
    draws(&frame.getShadowList()),
    parameters(std::move(pass_parameters))
{
	CHECK(parameters.depth && parameters.draw_binding && frame.valid(),
	    "Shadow pass requires depth and draw binding resources");
	CHECK(draws->getType() == DrawListType::Shadow,
	    "Shadow pass requires a shadow render list");
}

std::unique_ptr<ShadowPass> ShadowPass::create(
    PipelineResources& resources, const RenderFrame& frame,
    ShadowPassParams parameters)
{
	return std::unique_ptr<ShadowPass>(
	    new ShadowPass(resources, frame, std::move(parameters)));
}

void ShadowPass::setup(RDGPassBuilder& builder)
{
	RHIClearValue clear{};
	clear.setDepthStencil(1.0f);
	builder.setDepthAttachment(
	    parameters.depth,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store,
	    clear);
}

void ShadowPass::execute(RDGPassContext& context)
{
	auto& framebuffer = context.getFramebuffer();
	auto& pipeline = resources->getShadowPipeline(
	    framebuffer.getFramebufferInfo(), *frame,
	    *parameters.draw_binding->getLayout());
	RHIGraphicsState graphics_state{};
	graphics_state.setFramebuffer(&framebuffer)
	    .setPipeline(&pipeline)
	    .setViewport(shadowViewport(framebuffer.getDesc()))
	    .addBindingSet(parameters.draw_binding);
	if (parameters.enabled)
		submitDrawList(context.getCommand(), *draws, graphics_state,
		    nullptr, false);
}

}        // namespace Vortex
