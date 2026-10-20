export module Runtime.Render:GBuffer;

import Core;
import Runtime.RHI;
import Runtime.RDG;
import :Core;
import :Manager;

namespace Vortex {

class GBufferPass {
public:
	struct Params {
		RHIExtent extent{};
		RenderViewRefs view{};
		std::span<const RenderDraw> draws{};
	};

private:
	struct Data {
		GBuffer targets{};
		RenderViewRefs view{};
		RDGBindingSet bindings{};
		std::span<const RenderDraw> draws{};
		RHIRef<RHIBindingLayout> layout{};
		RHIRef<RHIGraphicsPipeline> pipeline{};
	};

	RHIRef<RHIBindingLayout> layout{};
	RHIRef<RHIGraphicsPipeline> pipeline{};

	static void setup(RDGPassBuilder& builder, Data& data);
	static void execute(RDGPassContext& context, const Data& data);

public:
	GBufferPass(RenderManager& resources);
	GBuffer build(RDGBuilder&, const Params&) const;
};

}        // namespace Vortex
