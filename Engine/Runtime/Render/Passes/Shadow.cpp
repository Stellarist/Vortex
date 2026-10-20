module;
#include <cstddef>
module Runtime.Render;

namespace Vortex {

static constexpr uint32 ShadowMapSize = 2048;
static constexpr float ShadowBias = 0.0015f;

struct ShadowConstants {
	Mat4 light_view_projection{1.0f};
	Vec4 params{};
};

static ShadowConstants makeConstants(const ShadowPass::Params& params)
{
	ShadowConstants result{};
	result.params = Vec4(ShadowBias, 1.0f / static_cast<float>(ShadowMapSize), 0.0f, 0.0f);
	if (!params.enabled || !params.direction)
		return result;

	const Vec3 direction = Math::safeNormalize(*params.direction);
	if (Math::isNearlyZero(direction))
		return result;

	const Vec3 center = params.camera_position;
	const Vec3 up = std::abs(direction.y) > 0.99f ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 1.0f, 0.0f);
	result.light_view_projection = Math::orthographic(-25.0f, 25.0f, -25.0f, 25.0f, 0.1f, 100.0f) *
	    Math::lookAt(center - direction * 40.0f, center, up);
	result.params.z = 1.0f;

	return result;
}


void ShadowPass::setup(RDGPassBuilder& builder, Data& data)
{
	data.bindings = RDGBindingBuilder(builder, *data.layout, "Shadow.Constants")
	    .constantBuffer(0, data.constants).build();
	RHIClearValue clear{};
	clear.setDepthStencil(1.0f);
	builder.setDepthAttachment(data.depth, RHILoadOp::Clear,
	    RHIStoreOp::Store, clear);
}

void ShadowPass::execute(RDGPassContext& context, const Data& data)
{
	if (!data.enabled || data.draws.empty())
		return;
	auto binding = data.bindings.resolve(context);
	auto& framebuffer = context.getFramebuffer();
	RHIGraphicsState state{};
	state.setFramebuffer(&framebuffer).setPipeline(data.pipeline.get()).setViewport(passViewport(framebuffer.getDesc())).addBindingSet(binding.get());
	drawFrameList(context.getCommand(), data.draws, state, nullptr);
}

ShadowPass::ShadowPass(RenderManager& resources)
{
	layout = &resources.bindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::pushConstants(sizeof(RenderDraw::PrimitiveConstants))));
	auto& input = resources.inputLayout(RHIInputLayoutDesc{}
	        .addBindingDesc({0, sizeof(MeshVertex), false})
	        .addAttributeDesc({"POSITION", RHIFormat::RGB32_FLOAT, 0, 0, offsetof(MeshVertex, pos)})
	        .addAttributeDesc({"TEXCOORD", RHIFormat::RG32_FLOAT, 1, 0, offsetof(MeshVertex, uv)})
	        .addAttributeDesc({"COLOR", RHIFormat::RGBA32_FLOAT, 2, 0, offsetof(MeshVertex, color)}));
	pipeline = &resources.graphicsPipeline(RHIGraphicsPipelineDesc{}
	        .setVertexShader(resources.shader("ShadowDepth.spv", RHIShaderType::Vertex, "vertexMain"))
	        .setPixelShader(resources.shader("ShadowDepth.spv", RHIShaderType::Pixel, "pixelMain"))
	        .setInputLayout(input)
	        .setRasterState(RHIRasterState{}.setCullMode(RHICullMode::Back).setDepthBias(1).setDepthBiasSlopeFactor(1.5f))
	        .setDepthStencilState(RHIDepthStencilState{}.setDepthTestEnable(true).setDepthWriteEnable(true).setDepthCompareOp(RHICompareOp::LessOrEqual))
	        .addBindingLayout(*layout)
	        .addBindingLayout(resources.materialLayout())
	        .setFramebufferInfo(RHIFramebufferInfo{}.setDepthFormat(RHIFormat::D32_FLOAT)));
}

ShadowMap ShadowPass::build(RDGBuilder& graph, const Params& params) const
{
	const auto constants = makeConstants(params);
	ShadowMap result{
	    .texture = graph.createTexture("Shadow.Map", RHITextureDesc{}.setWidth(ShadowMapSize).setHeight(ShadowMapSize).setFormat(RHIFormat::D32_FLOAT).setUsage(RHITextureUsage::DepthStencil | RHITextureUsage::Sampled)),
	    .constants = graph.createConstantBuffer("Shadow.Constants", constants),
	};

	graph.addPass(RDGPassDesc{.name = "Shadow.Depth", .type = RDGPassType::Raster},
	    Data{
	        .depth = result.texture,
	        .constants = result.constants,
	        .draws = params.draws,
	        .layout = layout,
	        .pipeline = pipeline,
	        .enabled = constants.params.z > 0.5f,
	    }, setup, execute);
	return result;
}

}        // namespace Vortex
