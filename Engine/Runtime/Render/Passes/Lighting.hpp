export module Runtime.Render:Lighting;

import Core;
import Runtime.RHI;
import Runtime.RDG;
import :Core;
import :Manager;

namespace Vortex {

class LightingPass {
public:
	struct Params {
		RHIExtent extent{};
		RenderViewRefs view{};
		GBuffer gbuffer{};
		ShadowMap shadow{};
		RHIFormat output_format{};
	};

private:
	struct Data {
		GBuffer gbuffer{};
		ShadowMap shadow{};
		RDGTextureRef color{};
		RenderViewRefs view{};
		RDGBindingSet view_bindings{}, shadow_bindings{};
		RDGBindingSet gbuffer_bindings{};
		RHIRef<RHIBindingLayout> view_layout{};
		RHIRef<RHIBindingLayout> gbuffer_layout{};
		RHIRef<RHIBindingLayout> shadow_layout{};
		RHIRef<RHISampler> sampler{};
		RHIRef<RHIGraphicsPipeline> pipeline{};
	};

	RenderManager* resources{};
	RHIRef<RHIBindingLayout> view_layout{}, gbuffer_layout{}, shadow_layout{};
	RHIRef<RHISampler> sampler{};
	RHIGraphicsPipelineDesc pipeline_desc{};

	static void setup(RDGPassBuilder& builder, Data& data);
	static void execute(RDGPassContext& context, const Data& data);

public:
	LightingPass(RenderManager& resources);
	RDGTextureRef build(RDGBuilder&, const Params&) const;
};

}        // namespace Vortex
