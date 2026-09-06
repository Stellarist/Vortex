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

static RHIRef<RHIBindingSet> createShadowBinding(RDGPassContext& context,
    RHIBindingLayout& layout, RHISampler& sampler, RHIBufferView& constants,
    RDGTextureViewRef shadow)
{
	auto binding = context.getDevice().createBindingSet(RHIBindingSetDesc{}
	                                                        .addItem(RHIBindingSetItem::constantBuffer(0, &constants))
	                                                        .addItem(RHIBindingSetItem::textureSRV(1, &context.getTextureView(shadow)))
	                                                        .addItem(RHIBindingSetItem::sampler(2, &sampler)),
	    layout);
	binding->setName("Shadow.Sample.BindingSet");
	return binding;
}

ForwardPass::ForwardPass(PipelineResources& pipeline_resources,
    const RenderFrame& frame, ForwardPassParams pass_parameters,
    bool is_transparent) :
    RDGPass(RDGPassDesc{.name = is_transparent ? "Forward.Transparent" : "Forward.Opaque", .type = RDGPassType::Raster}),
    resources(&pipeline_resources),
    frame(&frame),
    draws(is_transparent ? &frame.getTransparentList() : &frame.getOpaqueList()),
    parameters(std::move(pass_parameters)),
    transparent(is_transparent)
{
	CHECK(
	    parameters.targets.color &&
	        parameters.targets.depth && parameters.lighting.shadow_map &&
	        parameters.lighting.shadow_constants && frame.valid(),
	    "Forward pass requires scene, shadow, and draw resources");
	CHECK(draws->getType() == (transparent ? DrawListType::Transparent : DrawListType::Opaque),
	    "Forward pass received the wrong render list type");
}

std::unique_ptr<ForwardPass> ForwardPass::createOpaque(
    PipelineResources& resources, const RenderFrame& frame,
    ForwardPassParams parameters)
{
	return std::unique_ptr<ForwardPass>(
	    new ForwardPass(resources, frame, std::move(parameters), false));
}

std::unique_ptr<ForwardPass> ForwardPass::createTransparent(
    PipelineResources& resources, const RenderFrame& frame,
    ForwardPassParams parameters)
{
	return std::unique_ptr<ForwardPass>(
	    new ForwardPass(resources, frame, std::move(parameters), true));
}

void ForwardPass::setup(RDGPassBuilder& builder)
{
	shadow = builder.createTextureView(
	    "Forward.Shadow.SRV",
	    parameters.lighting.shadow_map);
	builder.read(shadow, ShaderResource);
	if (transparent) {
		builder.setColorAttachment(
		    0,
		    parameters.targets.color,
		    RHILoadOp::Load,
		    RHIStoreOp::Store);
		builder.setDepthReadOnlyAttachment(parameters.targets.depth);
		return;
	}

	RHIClearValue color_clear{};
	color_clear.setColor(RHIColor{0.1f, 0.1f, 0.1f, 1.0f});
	builder.setColorAttachment(
	    0,
	    parameters.targets.color,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store,
	    color_clear);
	RHIClearValue depth_clear{};
	depth_clear.setDepthStencil(1.0f);
	builder.setDepthAttachment(
	    parameters.targets.depth,
	    RHILoadOp::Clear,
	    RHIStoreOp::Store,
	    depth_clear);
}

void ForwardPass::execute(RDGPassContext& context)
{
	auto& shadow_layout = resources->getShadowSampleLayout();
	auto& sampler = resources->getPointClampSampler();
	auto& framebuffer = context.getFramebuffer();
	auto& pipeline = resources->getForwardPipeline(
	    framebuffer.getFramebufferInfo(), *frame, transparent);
	auto shadow_binding = createShadowBinding(context, shadow_layout, sampler,
	    *parameters.lighting.shadow_constants, shadow);

	RHIGraphicsState graphics_state{};
	graphics_state.setFramebuffer(&framebuffer)
	    .setPipeline(&pipeline)
	    .setViewport(sceneViewport(framebuffer.getDesc()));
	submitDrawList(context.getCommand(), *draws, graphics_state,
	    frame->getSceneBinding(), true, shadow_binding.get());
}

}        // namespace Vortex
