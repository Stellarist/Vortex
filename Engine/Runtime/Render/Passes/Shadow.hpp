export module Runtime.Render:Shadow;

import Core;
import Runtime.RHI;
import Runtime.RDG;
import :Core;
import :Manager;

namespace Vortex {

class ShadowPass {
public:
	struct Params {
		Vec3 camera_position{0.0f};
		std::optional<Vec3> direction{};
		bool enabled{};
		std::span<const RenderDraw> draws{};
	};

private:
	struct Data {
		RDGTextureRef depth{};
		RDGBufferRef constants{};
		RDGBindingSet bindings{};
		std::span<const RenderDraw> draws{};
		RHIRef<RHIBindingLayout> layout{};
		RHIRef<RHIGraphicsPipeline> pipeline{};
		bool enabled{};
	};

	RHIRef<RHIBindingLayout> layout{};
	RHIRef<RHIGraphicsPipeline> pipeline{};

	static void setup(RDGPassBuilder& builder, Data& data);
	static void execute(RDGPassContext& context, const Data& data);

public:
	ShadowPass(RenderManager& resources);
	ShadowMap build(RDGBuilder&, const Params&) const;
};

}        // namespace Vortex
