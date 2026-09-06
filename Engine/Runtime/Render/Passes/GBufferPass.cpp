module Runtime.Render;

namespace Vortex {

static RHIViewportState sceneViewport(const RHIFramebufferDesc& desc)
{
	RHIViewportState viewport{};
	viewport.addViewport(
	            RHIViewport(static_cast<float>(desc.width), static_cast<float>(desc.height)))
	    .addScissor(RHIRect(static_cast<int>(desc.width), static_cast<int>(desc.height)));
	return viewport;
}

GBufferPass::GBufferPass(PipelineResources& pipeline_resources,
    const RenderFrame& frame, GBufferTextures targets) :
    RDGPass(RDGPassDesc{.name = "Deferred.GBuffer", .type = RDGPassType::Raster}),
    resources(&pipeline_resources),
    frame(&frame),
    draws(&frame.getOpaqueList()),
    gbuffer(targets)
{
	CHECK(
	    gbuffer.albedo_metallic && gbuffer.normal_roughness &&
	        gbuffer.emissive_ao && gbuffer.depth &&
	        frame.valid(),
	    "GBuffer pass requires complete GBuffer attachments");
	CHECK(draws->getType() == DrawListType::Opaque,
	    "GBuffer pass requires an opaque render list");
}

std::unique_ptr<GBufferPass> GBufferPass::create(PipelineResources& resources,
    const RenderFrame& frame, GBufferTextures targets)
{
	return std::unique_ptr<GBufferPass>(
	    new GBufferPass(resources, frame, targets));
}

void GBufferPass::setup(RDGPassBuilder& builder)
{
	builder.setColorAttachment(
	    0,
	    gbuffer.albedo_metallic,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store);
	builder.setColorAttachment(
	    1,
	    gbuffer.normal_roughness,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store);
	builder.setColorAttachment(
	    2,
	    gbuffer.emissive_ao,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store);
	RHIClearValue depth_clear{};
	depth_clear.setDepthStencil(1.0f);
	builder.setDepthAttachment(
	    gbuffer.depth,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store,
	    depth_clear);
}

void GBufferPass::execute(RDGPassContext& context)
{
	auto& framebuffer = context.getFramebuffer();
	auto& pipeline = resources->getGBufferPipeline(
	    framebuffer.getFramebufferInfo(), *frame);
	RHIGraphicsState graphics_state{};
	graphics_state.setFramebuffer(&framebuffer)
	    .setPipeline(&pipeline)
	    .setViewport(sceneViewport(framebuffer.getDesc()));
	submitDrawList(context.getCommand(), *draws, graphics_state,
	    frame->getSceneBinding(), true);
}

}        // namespace Vortex
