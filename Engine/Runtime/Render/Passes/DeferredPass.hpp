export module Runtime.Render:Pass.Deferred;

import Core;
import :Pass;
import :Pipeline.Resources;
import Runtime.RDG;
import :Frame;

export namespace Vortex {

struct DeferredPassParams {
	GBufferTextures gbuffer{};
	LightingResources lighting{};
	RDGTextureRef color{};
};

class DeferredPass final : public RDGPass {
private:
	PipelineResources* resources{};
	const RenderFrame* frame{};

	DeferredPassParams parameters{};
	RDGTextureViewRef albedo_metallic{};
	RDGTextureViewRef normal_roughness{};
	RDGTextureViewRef emissive_ao{};
	RDGTextureViewRef depth{};
	RDGTextureViewRef shadow{};

	DeferredPass(PipelineResources& pipeline_resources,
	    const RenderFrame& frame, DeferredPassParams pass_parameters);

	void setup(RDGPassBuilder& builder) override;
	void execute(RDGPassContext& context) override;

public:
	static std::unique_ptr<DeferredPass> create(
	    PipelineResources& resources, const RenderFrame& frame,
	    DeferredPassParams parameters);
};

}        // namespace Vortex
