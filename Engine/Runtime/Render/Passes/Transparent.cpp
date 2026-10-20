module Runtime.Render;

namespace Vortex {

void TransparentPass::setup(RDGPassBuilder& builder, Data& data)
{
	data.view_bindings = makeViewBindings(builder, data.view, *data.view_layout, "Transparent.View");
	data.shadow_bindings = makeShadowBindings(builder, data.shadow, *data.shadow_layout, *data.sampler, "Transparent.Shadow");
	builder.setColorAttachment(0, data.color, RHILoadOp::Load, RHIStoreOp::Store);
	builder.setDepthReadOnlyAttachment(data.depth);
}

void TransparentPass::execute(RDGPassContext& context, const Data& data)
{
	auto view_binding = data.view_bindings.resolve(context);
	auto shadow_binding = data.shadow_bindings.resolve(context);
	auto& framebuffer = context.getFramebuffer();
	RHIGraphicsState state{};
	state.setFramebuffer(&framebuffer).setPipeline(data.pipeline.get()).setViewport(passViewport(framebuffer.getDesc()));
	drawFrameList(context.getCommand(), data.draws, state,
	    view_binding.get(), shadow_binding.get());
}

TransparentPass::TransparentPass(RenderManager& resources) :
    resources(&resources)
{
	view_layout = &viewBindingLayout(resources);
	shadow_layout = &shadowSampleLayout(resources);
	sampler = &screenSampler(resources);
	const auto blend = RHIColorBlendState::BlendDesc{}.setBlendEnable(true).setSrcBlend(RHIBlendFactor::SrcAlpha).setDstBlend(RHIBlendFactor::OneMinusSrcAlpha).setSrcBlendAlpha(RHIBlendFactor::One).setDstBlendAlpha(RHIBlendFactor::OneMinusSrcAlpha);
	pipeline_desc = RHIGraphicsPipelineDesc{}
	        .setVertexShader(resources.shader("Forward.spv", RHIShaderType::Vertex, "vertexMain"))
	        .setPixelShader(resources.shader("Forward.spv", RHIShaderType::Pixel, "pixelMain"))
	        .setInputLayout(meshInputLayout(resources))
	        .setRasterState(RHIRasterState{}.setCullMode(RHICullMode::None))
	        .setDepthStencilState(RHIDepthStencilState{}.setDepthTestEnable(true).setDepthWriteEnable(false).setDepthCompareOp(RHICompareOp::LessOrEqual))
	        .setBlendState(RHIColorBlendState{}.addBlendAttachment(blend))
	        .addBindingLayout(*view_layout)
	        .addBindingLayout(resources.materialLayout())
	        .addBindingLayout(*shadow_layout);
}

void TransparentPass::build(RDGBuilder& graph, const Params& params) const
{
	if (params.draws.empty())
		return;
	auto desc = pipeline_desc;
	desc.setFramebufferInfo(RHIFramebufferInfo{}.addColorFormat(params.color->desc.format).setDepthFormat(params.depth->desc.format));
	auto& pipeline = resources->graphicsPipeline(desc);
	graph.addPass(RDGPassDesc{.name = "Forward.Transparent", .type = RDGPassType::Raster},
	    Data{
	        .color = params.color,
	        .depth = params.depth,
	        .shadow = params.shadow,
	        .view = params.view,
	        .draws = params.draws,
	        .view_layout = view_layout,
	        .shadow_layout = shadow_layout,
	        .sampler = sampler,
	        .pipeline = &pipeline,
	    }, setup, execute);
}

}        // namespace Vortex
