module Runtime.Render;

import Runtime.RHI;
import Runtime.RDG;

namespace Vortex {

static RHITextureDesc sampledTargetDesc(const RHIExtent& extent, RHIFormat format)
{
	return RHITextureDesc{}
	    .setWidth(extent.width)
	    .setHeight(extent.height)
	    .setFormat(format)
	    .setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::Sampled);
}

static RHITextureDesc sceneColorDesc(const RHIExtent& extent, RHIFormat format)
{
	return RHITextureDesc{}
	    .setWidth(extent.width)
	    .setHeight(extent.height)
	    .setFormat(format)
	    .setUsage(RHITextureUsage::RenderTarget | RHITextureUsage::Sampled | RHITextureUsage::CopySource);
}

RenderPipeline::RenderPipeline(RHIDevice& new_device) :
    device(&new_device), resources(new_device)
{}

void RenderPipeline::ensureShadowResources()
{
	if (shadow_constant_buffer)
		return;

	shadow_constant_buffer = device->createBuffer(RHIBufferDesc{}.setSize(sizeof(ShadowData)).setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest).setAccess(RHIAccessMode::Write));
	shadow_constant_buffer->setName("Shadow.Constants");
	shadow_constant_view = device->createBufferView(RHIBufferViewDesc{}.setBuffer(shadow_constant_buffer.get()).setType(RHIBufferViewType::Constant));
	shadow_constant_view->setName("Shadow.Constants.CBV");
	auto layout = device->createBindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0)));
	shadow_draw_binding = device->createBindingSet(RHIBindingSetDesc{}
	                                                   .addItem(RHIBindingSetItem::constantBuffer(0, shadow_constant_view.get())),
	    *layout);
	shadow_draw_binding->setName("Shadow.Draw.BindingSet");
	LOG(Debug, "Initialized persistent directional shadow resources");
}

void RenderPipeline::updateShadow(
    const RenderFrame& frame, bool enabled, float bias)
{
	CHECK(std::isfinite(bias) && bias >= 0.0f && bias <= 0.05f,
	    "Directional shadow bias must be finite and in [0, 0.05]");
	ensureShadowResources();

	ShadowData data{};
	data.light_view_projection = Mat4(1.0f);
	data.params = Vec4(
	    bias,
	    1.0f / static_cast<float>(SHADOW_MAP_SIZE),
	    0.0f,
	    0.0f);
	const auto& light_direction = frame.getMainDirectionalLightDirection();
	if (enabled && light_direction) {
		const Vec3 direction = Math::safeNormalize(*light_direction);
		if (!Math::isNearlyZero(direction)) {
			const auto& view = frame.getView();
			const Vec3 center = view.hasCamera() ? view.getPosition() : Vec3(0.0f);
			const Vec3 up = std::abs(direction.y) > 0.99f ?
			    Vec3(1.0f, 0.0f, 0.0f) :
			    Vec3(0.0f, 1.0f, 0.0f);
			const Mat4 light_view = Math::lookAt(
			    center - direction * 40.0f, center, up);
			const Mat4 projection = Math::orthographic(
			    -25.0f,
			    25.0f,
			    -25.0f,
			    25.0f,
			    0.1f,
			    100.0f);
			data.light_view_projection = projection * light_view;
			data.params.z = 1.0f;
		}
	}
	shadow_enabled = data.params.z > 0.5f;

	void* mapped = device->mapBuffer(shadow_constant_buffer.get(), RHIAccessMode::Write);
	CHECK(mapped, "Failed to map the directional shadow constant buffer");
	std::memcpy(mapped, &data, sizeof(data));
	device->unmapBuffer(shadow_constant_buffer.get());
}

LightingResources RenderPipeline::addShadowPass(
    RDGBuilder& graph, const RenderFrame& frame, const RenderSettings& settings)
{
	updateShadow(frame, settings.directional_shadows, settings.shadow_bias);
	auto shadow_map = graph.createTexture("ShadowMap",
	    RHITextureDesc{}.setWidth(SHADOW_MAP_SIZE).setHeight(SHADOW_MAP_SIZE).setFormat(RHIFormat::D32_FLOAT).setUsage(RHITextureUsage::DepthStencil | RHITextureUsage::Sampled));
	graph.addPass(ShadowPass::create(resources, frame,
	    ShadowPassParams{
	        .depth = shadow_map,
	        .draw_binding = shadow_draw_binding.get(),
	        .enabled = shadow_enabled,
	    }));
	return {.shadow_map = shadow_map, .shadow_constants = shadow_constant_view.get()};
}

SceneTextures RenderPipeline::buildForwardPath(
    RDGBuilder& graph, const RenderFrame& frame, const RHIExtent& extent,
    RHIFormat color_format, const LightingResources& lighting)
{
	SceneTextures targets{
	    .color = graph.createTexture("SceneColor", sceneColorDesc(extent, color_format)),
	    .depth = graph.createTexture("SceneDepth",
	        RHITextureDesc{}.setWidth(extent.width).setHeight(extent.height).setFormat(RHIFormat::D32_FLOAT).setUsage(RHITextureUsage::DepthStencil)),
	};
	graph.addPass(ForwardPass::createOpaque(resources, frame,
	    ForwardPassParams{.targets = targets, .lighting = lighting}));
	return targets;
}

SceneTextures RenderPipeline::buildDeferredPath(
    RDGBuilder& graph, const RenderFrame& frame, const RHIExtent& extent,
    RHIFormat color_format, const LightingResources& lighting)
{
	SceneTextures targets{
	    .color = graph.createTexture("SceneColor", sceneColorDesc(extent, color_format)),
	    .depth = graph.createTexture("SceneDepth",
	        RHITextureDesc{}.setWidth(extent.width).setHeight(extent.height).setFormat(RHIFormat::D32_FLOAT).setUsage(RHITextureUsage::DepthStencil | RHITextureUsage::Sampled)),
	};
	GBufferTextures gbuffer{
	    .albedo_metallic = graph.createTexture("GBuffer.AlbedoMetallic",
	        sampledTargetDesc(extent, RHIFormat::RGBA8_SRGB)),
	    .normal_roughness = graph.createTexture("GBuffer.NormalRoughness",
	        sampledTargetDesc(extent, RHIFormat::RGBA16_FLOAT)),
	    .emissive_ao = graph.createTexture("GBuffer.EmissiveAO",
	        sampledTargetDesc(extent, RHIFormat::RGBA16_FLOAT)),
	    .depth = targets.depth,
	};
	graph.addPass(GBufferPass::create(resources, frame, gbuffer));
	graph.addPass(DeferredPass::create(resources, frame,
	    DeferredPassParams{
	        .gbuffer = gbuffer,
	        .lighting = lighting,
	        .color = targets.color,
	    }));
	return targets;
}

void RenderPipeline::build(RDGBuilder& graph, RDGTextureRef output,
    const RenderFrame& frame, const RenderSettings& settings, const RHIExtent& extent)
{
	CHECK(output && extent.width != 0 && extent.height != 0,
	    "Render pipeline requires an output texture and non-empty extent");
	CHECK(settings.render_path == RenderPathType::Forward ||
	        settings.render_path == RenderPathType::Deferred,
	    "Unknown render path");

	const auto lighting = addShadowPass(graph, frame, settings);
	const auto targets = settings.render_path == RenderPathType::Forward ?
	    buildForwardPath(graph, frame, extent, output->desc.format, lighting) :
	    buildDeferredPath(graph, frame, extent, output->desc.format, lighting);

	if (frame.getTransparentList().getDrawCount() > 0)
		graph.addPass(ForwardPass::createTransparent(resources, frame,
		    ForwardPassParams{.targets = targets, .lighting = lighting}));

	graph.addPass(BlitPass::create(
	    BlitPassParams{.source = targets.color, .destination = output}));
	graph.addOutput(output);
}

}        // namespace Vortex
