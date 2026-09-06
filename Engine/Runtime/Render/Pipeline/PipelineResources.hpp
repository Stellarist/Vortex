export module Runtime.Render:Pipeline.Resources;

import Core;
import Runtime.RHI;
import :Frame;
import :Scene.Resource;

export namespace Vortex {

class PipelineResources {
private:
	enum class Shader : uint8 {
		ShadowDepth,
		Forward,
		Geometry,
		Deferred,
		Count,
	};

	enum class PipelineType : uint8 {
		ForwardOpaque,
		ForwardTransparent,
		GBuffer,
		Deferred,
		Shadow,
	};

	struct PipelineEntry {
		PipelineType type{};
		RHIFramebufferInfo framebuffer{};
		std::array<const RHIBindingLayout*, 4> layouts{};
		RHIRef<RHIGraphicsPipeline> pipeline;
	};

	RHIDevice* device{};
	std::array<std::array<RHIRef<RHIShader>, 2>, static_cast<size_t>(Shader::Count)> shaders{};
	RHIRef<RHIInputLayout> mesh_input_layout;
	RHIRef<RHIInputLayout> shadow_input_layout;
	RHIRef<RHISampler> point_clamp_sampler;
	RHIRef<RHIBindingLayout> shadow_sample_layout;
	RHIRef<RHIBindingLayout> gbuffer_sample_layout;
	std::vector<PipelineEntry> pipelines;

	RHIGraphicsPipeline* findPipeline(PipelineType type,
	    const RHIFramebufferInfo& framebuffer,
	    const std::array<const RHIBindingLayout*, 4>& layouts) const noexcept;
	RHIGraphicsPipeline& storePipeline(PipelineType type,
	    const RHIFramebufferInfo& framebuffer,
	    std::array<const RHIBindingLayout*, 4> layouts,
	    RHIRef<RHIGraphicsPipeline> pipeline);
	RHIShader& getShader(Shader shader, RHIShaderType stage);

public:
	PipelineResources(RHIDevice& device);
	void clearPipelines() noexcept { pipelines.clear(); }

	RHISampler& getPointClampSampler() const noexcept { return *point_clamp_sampler; }
	RHIBindingLayout& getShadowSampleLayout() const noexcept { return *shadow_sample_layout; }
	RHIBindingLayout& getGBufferSampleLayout() const noexcept { return *gbuffer_sample_layout; }

	RHIGraphicsPipeline& getForwardPipeline(const RHIFramebufferInfo& framebuffer,
	    const RenderFrame& frame, bool transparent);
	RHIGraphicsPipeline& getGBufferPipeline(const RHIFramebufferInfo& framebuffer,
	    const RenderFrame& frame);
	RHIGraphicsPipeline& getDeferredPipeline(const RHIFramebufferInfo& framebuffer,
	    const RenderFrame& frame);
	RHIGraphicsPipeline& getShadowPipeline(const RHIFramebufferInfo& framebuffer,
	    const RenderFrame& frame, const RHIBindingLayout& shadow_draw_layout);
};

}        // namespace Vortex
