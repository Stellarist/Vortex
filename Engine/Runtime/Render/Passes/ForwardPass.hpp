export module Runtime.Render:Pass.Forward;

import Core;
import :Pass;
import :Pipeline.Resources;
import Runtime.RDG;
import :Frame;

export namespace Vortex {

struct ForwardPassParams {
	SceneTextures targets{};
	LightingResources lighting{};
};

class ForwardPass final : public RDGPass {
private:
	PipelineResources* resources{};
	const RenderFrame* frame{};
	const DrawList* draws{};

	ForwardPassParams parameters{};
	RDGTextureViewRef shadow{};
	bool transparent{};

	ForwardPass(PipelineResources& pipeline_resources, const RenderFrame& frame,
	    ForwardPassParams pass_parameters, bool is_transparent);

	void setup(RDGPassBuilder& builder) override;
	void execute(RDGPassContext& context) override;

public:
	static std::unique_ptr<ForwardPass> createOpaque(PipelineResources& resources,
	    const RenderFrame& frame, ForwardPassParams parameters);
	static std::unique_ptr<ForwardPass> createTransparent(
	    PipelineResources& resources, const RenderFrame& frame,
	    ForwardPassParams parameters);
};

}        // namespace Vortex
