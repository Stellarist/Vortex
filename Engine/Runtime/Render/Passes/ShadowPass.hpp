export module Runtime.Render:Pass.Shadow;

import Core;
import :Pass;
import :Pipeline.Resources;
import Runtime.RDG;
import :Frame;

export namespace Vortex {

inline constexpr uint32 SHADOW_MAP_SIZE = 2048;

struct ShadowData {
	Mat4 light_view_projection{1.0f};
	Vec4 params{};
};

struct ShadowPassParams {
	RDGTextureRef depth{};
	RHIBindingSet* draw_binding{};
	bool enabled{};
};

class ShadowPass final : public RDGPass {
private:
	PipelineResources* resources{};
	const RenderFrame* frame{};
	const DrawList* draws{};

	ShadowPassParams parameters{};

	ShadowPass(PipelineResources& pipeline_resources,
	    const RenderFrame& frame, ShadowPassParams pass_parameters);

	void setup(RDGPassBuilder& builder) override;
	void execute(RDGPassContext& context) override;

public:
	static std::unique_ptr<ShadowPass> create(
	    PipelineResources& resources, const RenderFrame& frame,
	    ShadowPassParams parameters);
};

}        // namespace Vortex
