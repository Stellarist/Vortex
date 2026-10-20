export module Runtime.Render:Renderer;

import Core;
import Runtime.World;
import Runtime.RHI;
import Runtime.RDG;
import :Core;
import :Scene;
import :Manager;
import :Shadow;
import :GBuffer;
import :Lighting;
import :Transparent;
import :Present;

export namespace Vortex {

struct RenderFrame {
	RenderPreparedView prepared;
	RenderSettings settings;
	RenderStats stats{};
	RenderViewRefs view{};
	RDGTextureRef output{};
	bool executed{};
	RDGBuilder graph;

	RenderFrame(RenderPreparedView prepared, RenderSettings settings, RHITexture& target, RDGResourcePool& pool);

	RenderFrame(const RenderFrame&) = delete;
	RenderFrame& operator=(const RenderFrame&) = delete;

	RenderFrame(RenderFrame&&) = delete;
	RenderFrame& operator=(RenderFrame&&) = delete;

	void execute(RHIDevice& device, RHICommandList& command);
};

struct RenderFrameRequest {
	const World* world{};
	std::function<void(RHICommandList&)> overlay{};
};


class Renderer {
private:
	RHIContext* context{};
	RenderSettings settings{};
	RenderStats stats{};
	RenderManager resources;
	RDGResourcePool resource_pool;
	bool frame_active{};

protected:
	Renderer(RHIContext& context, RenderSettings settings = {});

	RenderManager& getResources() noexcept { return resources; }

	virtual void buildGraph(RenderFrame& frame) = 0;

public:
	virtual ~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	bool beginFrame();
	void render(const RenderFrameRequest& request);
	void waitIdle();

	RHIContext& getContext() const noexcept;
	RenderSettings& getSettings() noexcept;
	const RenderStats& getStats() const noexcept;
	RDGResourceStats getResourcePoolStats() const noexcept { return resource_pool.getStats(); }
};


class DeferredRenderer final : public Renderer {
	ShadowPass shadow_pass;
	GBufferPass gbuffer_pass;
	LightingPass lighting_pass;
	TransparentPass transparent_pass;
	PresentPass present_pass;

protected:
	void buildGraph(RenderFrame& frame) override;

public:
	DeferredRenderer(RHIContext& context, RenderSettings settings = {});
};

}        // namespace Vortex
