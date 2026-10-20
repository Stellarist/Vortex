module Runtime.Render;

namespace Vortex {

void LightingPass::setup(RDGPassBuilder& builder, Data& data)
{
	data.view_bindings = makeViewBindings(builder, data.view, *data.view_layout, "Lighting.View");
	data.shadow_bindings = makeShadowBindings(builder, data.shadow, *data.shadow_layout, *data.sampler, "Lighting.Shadow");
	data.gbuffer_bindings = RDGBindingBuilder(builder, *data.gbuffer_layout, "Lighting.GBuffer")
	    .textureSRV(0, data.gbuffer.albedo)
	    .textureSRV(1, data.gbuffer.normal)
	    .textureSRV(2, data.gbuffer.emissive)
	    .textureSRV(3, data.gbuffer.depth)
	    .sampler(4, *data.sampler)
	    .build();
	builder.setColorAttachment(0, data.color, RHILoadOp::Clear, RHIStoreOp::Store);
}

void LightingPass::execute(RDGPassContext& context, const Data& data)
{
	auto view_binding = data.view_bindings.resolve(context);
	auto shadow_binding = data.shadow_bindings.resolve(context);
	auto gbuffer_binding = data.gbuffer_bindings.resolve(context);
	auto& framebuffer = context.getFramebuffer();
	context.getCommand().setGraphicsState(RHIGraphicsState{}
	        .setFramebuffer(&framebuffer)
	        .setPipeline(data.pipeline.get())
	        .setViewport(passViewport(framebuffer.getDesc()))
	        .addBindingSet(view_binding.get())
	        .addBindingSet(gbuffer_binding.get())
	        .addBindingSet(shadow_binding.get()));
	context.getCommand().draw(RHIDrawArguments{}
	        .setVertexCount(3)
	        .setInstanceCount(1));
}

LightingPass::LightingPass(RenderManager& resources) :
    resources(&resources)
{
	view_layout = &viewBindingLayout(resources);
	shadow_layout = &shadowSampleLayout(resources);
	sampler = &screenSampler(resources);
	gbuffer_layout = &resources.bindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::textureSRV(0))
	        .addItem(RHIBindingLayoutItem::textureSRV(1))
	        .addItem(RHIBindingLayoutItem::textureSRV(2))
	        .addItem(RHIBindingLayoutItem::textureSRV(3))
	        .addItem(RHIBindingLayoutItem::sampler(4)));
	pipeline_desc = RHIGraphicsPipelineDesc{}
	        .setVertexShader(resources.shader("Deferred.spv", RHIShaderType::Vertex, "vertexMain"))
	        .setPixelShader(resources.shader("Deferred.spv", RHIShaderType::Pixel, "pixelMain"))
	        .setRasterState(RHIRasterState{}.setCullMode(RHICullMode::None))
	        .addBindingLayout(*view_layout)
	        .addBindingLayout(*gbuffer_layout)
	        .addBindingLayout(*shadow_layout);
}

RDGTextureRef LightingPass::build(RDGBuilder& graph, const Params& params) const
{
	auto color = graph.createTexture("Scene.Color", RHITextureDesc{}.setWidth(params.extent.width).setHeight(params.extent.height).setFormat(params.output_format).setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::TransferSource));
	auto desc = pipeline_desc;
	desc.setFramebufferInfo(RHIFramebufferInfo{}.addColorFormat(params.output_format));
	auto& pipeline = resources->graphicsPipeline(desc);
	graph.addPass(RDGPassDesc{.name = "Deferred.Lighting", .type = RDGPassType::Raster},
	    Data{
	        .gbuffer = params.gbuffer,
	        .shadow = params.shadow,
	        .color = color,
	        .view = params.view,
	        .view_layout = view_layout,
	        .gbuffer_layout = gbuffer_layout,
	        .shadow_layout = shadow_layout,
	        .sampler = sampler,
	        .pipeline = &pipeline,
	    }, setup, execute);
	return color;
}

}        // namespace Vortex
