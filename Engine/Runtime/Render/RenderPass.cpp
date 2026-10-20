module;
#include <cstddef>
module Runtime.Render;

namespace Vortex {

RDGBindingSet makeViewBindings(RDGPassBuilder& builder, RenderViewRefs refs,
    RHIBindingLayout& layout, std::string_view name)
{
	return RDGBindingBuilder(builder, layout, name)
	    .constantBuffer(0, refs.constants)
	    .structuredBufferSRV(1, refs.lights)
	    .build();
}

RDGBindingSet makeShadowBindings(RDGPassBuilder& builder, ShadowMap refs,
    RHIBindingLayout& layout, RHISampler& sampler, std::string_view name)
{
	return RDGBindingBuilder(builder, layout, name)
	    .constantBuffer(0, refs.constants)
	    .textureSRV(1, refs.texture)
	    .sampler(2, sampler)
	    .build();
}

RHIViewportState passViewport(const RHIFramebufferDesc& framebuffer)
{
	return RHIViewportState{}
	    .addViewport(RHIViewport(static_cast<float>(framebuffer.width),
	        static_cast<float>(framebuffer.height)))
	    .addScissor(RHIRect(static_cast<int>(framebuffer.width),
	        static_cast<int>(framebuffer.height)));
}

void drawFrameList(RHICommandList& command, std::span<const RenderDraw> draws,
    const RHIGraphicsState& base, RHIBindingSet* view,
    RHIBindingSet* trailing)
{
	for (const auto& draw : draws) {
		auto state = base;
		if (view)
			state.addBindingSet(view);
		state.addBindingSet(draw.material.get());
		if (trailing)
			state.addBindingSet(trailing);
		state.addVertexBuffer(draw.vertices).setIndexBuffer(draw.indices);
		command.setGraphicsState(state);
		command.setPushConstants(&draw.transform, sizeof(draw.transform));
		command.drawIndexed(draw.arguments);
	}
}


RHIInputLayout& meshInputLayout(RenderManager& resources)
{
	return resources.inputLayout(RHIInputLayoutDesc{}
	        .addBindingDesc({0, sizeof(MeshVertex), false})
	        .addAttributeDesc({"POSITION", RHIFormat::RGB32_FLOAT, 0, 0, offsetof(MeshVertex, pos)})
	        .addAttributeDesc({"NORMAL", RHIFormat::RGB32_FLOAT, 1, 0, offsetof(MeshVertex, normal)})
	        .addAttributeDesc({"TEXCOORD", RHIFormat::RG32_FLOAT, 2, 0, offsetof(MeshVertex, uv)})
	        .addAttributeDesc({"COLOR", RHIFormat::RGBA32_FLOAT, 3, 0, offsetof(MeshVertex, color)}));
}

RHIBindingLayout& viewBindingLayout(RenderManager& resources)
{
	return resources.bindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::structuredBufferSRV(1))
	        .addItem(RHIBindingLayoutItem::pushConstants(sizeof(RenderDraw::PrimitiveConstants))));
}

RHIBindingLayout& shadowSampleLayout(RenderManager& resources)
{
	return resources.bindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::textureSRV(1))
	        .addItem(RHIBindingLayoutItem::sampler(2)));
}

RHISampler& screenSampler(RenderManager& resources)
{
	return resources.sampler(RHISamplerDesc{}
	        .setAllFilters(false)
	        .setAllAddressModes(RHISamplerAddress::ClampToEdge));
}

}        // namespace Vortex
