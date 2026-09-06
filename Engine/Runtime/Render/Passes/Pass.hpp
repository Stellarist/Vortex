export module Runtime.Render:Pass;

export import Runtime.RDG;
import :Frame;

export namespace Vortex {

struct SceneTextures {
	RDGTextureRef color{};
	RDGTextureRef depth{};
};

struct GBufferTextures {
	RDGTextureRef albedo_metallic{};
	RDGTextureRef normal_roughness{};
	RDGTextureRef emissive_ao{};
	RDGTextureRef depth{};
};

struct LightingResources {
	RDGTextureRef shadow_map{};
	RHIBufferView* shadow_constants{};
};

void submitDrawList(RHICommandList& command, const DrawList& draws,
    const RHIGraphicsState& base_state, RHIBindingSet* scene_binding,
    bool bind_material, RHIBindingSet* trailing_binding = nullptr);

}        // namespace Vortex
