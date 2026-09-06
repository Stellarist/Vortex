export module Runtime.Render:Pipeline;

import Core;
import Runtime.RDG;
import :Settings;
import :Frame;
import :Pipeline.Resources;
import :Pass;
import :Pass.Forward;
import :Pass.GBuffer;
import :Pass.Deferred;
import :Pass.Shadow;
import :Pass.Blit;
import Runtime.RHI;

export namespace Vortex {

class RenderPipeline {
	RHIDevice* device{};
	PipelineResources resources;

	RHIRef<RHIBuffer> shadow_constant_buffer{};
	RHIRef<RHIBufferView> shadow_constant_view{};
	RHIRef<RHIBindingSet> shadow_draw_binding{};
	bool shadow_enabled{};

	void ensureShadowResources();
	void updateShadow(const RenderFrame& frame, bool enabled, float bias);
	LightingResources addShadowPass(RDGBuilder& graph, const RenderFrame& frame,
	    const RenderSettings& settings);
	SceneTextures buildForwardPath(RDGBuilder& graph, const RenderFrame& frame,
	    const RHIExtent& extent, RHIFormat color_format, const LightingResources& lighting);
	SceneTextures buildDeferredPath(RDGBuilder& graph, const RenderFrame& frame,
	    const RHIExtent& extent, RHIFormat color_format, const LightingResources& lighting);

public:
	RenderPipeline(RHIDevice& device);
	void resetSceneState() noexcept { resources.clearPipelines(); }

	void build(RDGBuilder& graph, RDGTextureRef output, const RenderFrame& frame,
	    const RenderSettings& settings, const RHIExtent& extent);
};

}        // namespace Vortex
