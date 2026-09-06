module Runtime.Render;

namespace Vortex {

static RHIViewportState lightingViewport(const RHIFramebufferDesc& desc)
{
	RHIViewportState viewport{};
	viewport.addViewport(
	            RHIViewport(static_cast<float>(desc.width), static_cast<float>(desc.height)))
	    .addScissor(RHIRect(static_cast<int>(desc.width), static_cast<int>(desc.height)));
	return viewport;
}

DeferredPass::DeferredPass(PipelineResources& pipeline_resources,
    const RenderFrame& frame, DeferredPassParams pass_parameters) :
    RDGPass(RDGPassDesc{.name = "Deferred.Lighting", .type = RDGPassType::Raster}),
    resources(&pipeline_resources),
    frame(&frame),
    parameters(std::move(pass_parameters))
{
	const auto& gbuffer = parameters.gbuffer;
	const auto& lighting = parameters.lighting;
	CHECK(
	    gbuffer.albedo_metallic && gbuffer.normal_roughness &&
	        gbuffer.emissive_ao && gbuffer.depth && lighting.shadow_map &&
	        parameters.color && lighting.shadow_constants && frame.valid(),
	    "Deferred lighting requires GBuffer and shadow resources");
}

std::unique_ptr<DeferredPass> DeferredPass::create(
    PipelineResources& resources, const RenderFrame& frame,
    DeferredPassParams parameters)
{
	return std::unique_ptr<DeferredPass>(
	    new DeferredPass(resources, frame, std::move(parameters)));
}

void DeferredPass::setup(RDGPassBuilder& builder)
{
	albedo_metallic = builder.createTextureView(
	    "Deferred.GBuffer.AlbedoMetallic.SRV",
	    parameters.gbuffer.albedo_metallic);
	normal_roughness = builder.createTextureView(
	    "Deferred.GBuffer.NormalRoughness.SRV",
	    parameters.gbuffer.normal_roughness);
	emissive_ao = builder.createTextureView(
	    "Deferred.GBuffer.EmissiveAO.SRV",
	    parameters.gbuffer.emissive_ao);
	depth = builder.createTextureView(
	    "Deferred.GBuffer.Depth.SRV",
	    parameters.gbuffer.depth);
	shadow = builder.createTextureView(
	    "Deferred.Shadow.SRV",
	    parameters.lighting.shadow_map);
	builder.read(albedo_metallic, ShaderResource);
	builder.read(normal_roughness, ShaderResource);
	builder.read(emissive_ao, ShaderResource);
	builder.read(depth, ShaderResource);
	builder.read(shadow, ShaderResource);
	builder.setColorAttachment(
	    0,
	    parameters.color,
	    RHILoadOp::Discard,
	    RHIStoreOp::Store);
}

void DeferredPass::execute(RDGPassContext& context)
{
	auto& device = context.getDevice();
	auto& sampler = resources->getPointClampSampler();
	auto& gbuffer_layout = resources->getGBufferSampleLayout();
	auto& shadow_layout = resources->getShadowSampleLayout();
	auto& framebuffer = context.getFramebuffer();
	auto& pipeline = resources->getDeferredPipeline(
	    framebuffer.getFramebufferInfo(), *frame);
	auto gbuffer_binding = device.createBindingSet(RHIBindingSetDesc{}
	                                                   .addItem(RHIBindingSetItem::textureSRV(0, &context.getTextureView(albedo_metallic)))
	                                                   .addItem(RHIBindingSetItem::textureSRV(1, &context.getTextureView(normal_roughness)))
	                                                   .addItem(RHIBindingSetItem::textureSRV(2, &context.getTextureView(emissive_ao)))
	                                                   .addItem(RHIBindingSetItem::textureSRV(3, &context.getTextureView(depth)))
	                                                   .addItem(RHIBindingSetItem::sampler(4, &sampler)),
	    gbuffer_layout);
	gbuffer_binding->setName("Deferred.GBuffer.BindingSet");
	auto shadow_binding = device.createBindingSet(RHIBindingSetDesc{}
	                                                  .addItem(RHIBindingSetItem::constantBuffer(0, parameters.lighting.shadow_constants))
	                                                  .addItem(RHIBindingSetItem::textureSRV(1, &context.getTextureView(shadow)))
	                                                  .addItem(RHIBindingSetItem::sampler(2, &sampler)),
	    shadow_layout);
	shadow_binding->setName("Shadow.Sample.BindingSet");
	context.getCommand().setGraphicsState(RHIGraphicsState{}.setFramebuffer(&framebuffer).setPipeline(&pipeline).setViewport(lightingViewport(framebuffer.getDesc())).addBindingSet(frame->getSceneBinding()).addBindingSet(gbuffer_binding.get()).addBindingSet(shadow_binding.get()));
	context.getCommand().draw(
	    RHIDrawArguments{}.setVertexCount(3).setInstanceCount(1));
}

}        // namespace Vortex
