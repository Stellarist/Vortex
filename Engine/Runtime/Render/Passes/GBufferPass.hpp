export module Runtime.Render:Pass.GBuffer;

import Core;
import :Pass;
import :Pipeline.Resources;
import Runtime.RDG;
import :Frame;

export namespace Vortex {

class GBufferPass final : public RDGPass {
private:
	PipelineResources* resources{};
	const RenderFrame* frame{};
	const DrawList* draws{};

	GBufferTextures gbuffer{};

	GBufferPass(PipelineResources& pipeline_resources,
	    const RenderFrame& frame,
	    GBufferTextures targets);

	void setup(RDGPassBuilder& builder) override;
	void execute(RDGPassContext& context) override;

public:
	static std::unique_ptr<GBufferPass> create(PipelineResources& resources,
	    const RenderFrame& frame,
	    GBufferTextures targets);
};

}        // namespace Vortex
