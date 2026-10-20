module Runtime.Render;

namespace Vortex {

void GBufferPass::setup(RDGPassBuilder& builder, Data& data)
{
	data.bindings = makeViewBindings(builder, data.view, *data.layout, "GBuffer.View");
	builder.setColorAttachment(0, data.targets.albedo,
	    RHILoadOp::Clear, RHIStoreOp::Store);
	builder.setColorAttachment(1, data.targets.normal,
	    RHILoadOp::Clear, RHIStoreOp::Store);
	builder.setColorAttachment(2, data.targets.emissive,
	    RHILoadOp::Clear, RHIStoreOp::Store);
	RHIClearValue depth_clear{};
	depth_clear.setDepthStencil(1.0f);
	builder.setDepthAttachment(data.targets.depth,
	    RHILoadOp::Clear, RHIStoreOp::Store, depth_clear);
}

void GBufferPass::execute(RDGPassContext& context, const Data& data)
{
	if (data.draws.empty())
		return;
	auto binding = data.bindings.resolve(context);
	auto& framebuffer = context.getFramebuffer();
	RHIGraphicsState state{};
	state.setFramebuffer(&framebuffer).setPipeline(data.pipeline.get()).setViewport(passViewport(framebuffer.getDesc()));
	drawFrameList(context.getCommand(), data.draws, state, binding.get());
}

GBufferPass::GBufferPass(RenderManager& resources)
{
	layout = &viewBindingLayout(resources);
	pipeline = &resources.graphicsPipeline(RHIGraphicsPipelineDesc{}
	        .setVertexShader(resources.shader("Geometry.spv", RHIShaderType::Vertex, "vertexMain"))
	        .setPixelShader(resources.shader("Geometry.spv", RHIShaderType::Pixel, "pixelMain"))
	        .setInputLayout(meshInputLayout(resources))
	        .setRasterState(RHIRasterState{}.setCullMode(RHICullMode::None))
	        .setDepthStencilState(RHIDepthStencilState{}.setDepthTestEnable(true).setDepthWriteEnable(true).setDepthCompareOp(RHICompareOp::LessOrEqual))
	        .addBindingLayout(*layout)
	        .addBindingLayout(resources.materialLayout())
	        .setFramebufferInfo(RHIFramebufferInfo{}
	                .addColorFormat(RHIFormat::RGBA8_SRGB)
	                .addColorFormat(RHIFormat::RGBA16_FLOAT)
	                .addColorFormat(RHIFormat::RGBA16_FLOAT)
	                .setDepthFormat(RHIFormat::D32_FLOAT)));
}

GBuffer GBufferPass::build(RDGBuilder& graph, const Params& params) const
{
	const auto makeColor = [&](RHIFormat format) {
		return RHITextureDesc{}
		    .setWidth(params.extent.width)
		    .setHeight(params.extent.height)
		    .setFormat(format)
		    .setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::Sampled);
	};
	GBuffer result{
	    .albedo = graph.createTexture("GBuffer.AlbedoMetallic", makeColor(RHIFormat::RGBA8_SRGB)),
	    .normal = graph.createTexture("GBuffer.NormalRoughness", makeColor(RHIFormat::RGBA16_FLOAT)),
	    .emissive = graph.createTexture("GBuffer.EmissiveAO", makeColor(RHIFormat::RGBA16_FLOAT)),
	    .depth = graph.createTexture("GBuffer.Depth", RHITextureDesc{}.setWidth(params.extent.width).setHeight(params.extent.height).setFormat(RHIFormat::D32_FLOAT).setUsage(RHITextureUsage::DepthStencil | RHITextureUsage::Sampled)),
	};
	graph.addPass(RDGPassDesc{.name = "GBuffer.Geometry", .type = RDGPassType::Raster},
	    Data{.targets = result, .view = params.view, .draws = params.draws, .layout = layout, .pipeline = pipeline}, setup, execute);
	return result;
}

}        // namespace Vortex
