export module Runtime.Render:Transparent;

import Core;
import Runtime.RHI;
import Runtime.RDG;
import :Core;
import :Manager;

namespace Vortex {

class TransparentPass {
public:
	struct Params {
		RenderViewRefs view{};
		RDGTextureRef color{};
		RDGTextureRef depth{};
		ShadowMap shadow{};
		std::span<const RenderDraw> draws{};
	};

private:
	struct Data {
		RDGTextureRef color{};
		RDGTextureRef depth{};
		ShadowMap shadow{};
		RenderViewRefs view{};
		RDGBindingSet view_bindings{}, shadow_bindings{};
		std::span<const RenderDraw> draws{};
		RHIRef<RHIBindingLayout> view_layout{};
		RHIRef<RHIBindingLayout> shadow_layout{};
		RHIRef<RHISampler> sampler{};
		RHIRef<RHIGraphicsPipeline> pipeline{};
	};

	RenderManager* resources{};
	RHIRef<RHIBindingLayout> view_layout{}, shadow_layout{};
	RHIRef<RHISampler> sampler{};
	RHIGraphicsPipelineDesc pipeline_desc{};

	static void setup(RDGPassBuilder& builder, Data& data);
	static void execute(RDGPassContext& context, const Data& data);

public:
	TransparentPass(RenderManager& resources);
	void build(RDGBuilder&, const Params&) const;
};

}        // namespace Vortex
