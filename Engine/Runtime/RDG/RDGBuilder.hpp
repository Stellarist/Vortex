export module Runtime.RDG:Builder;

import Core;
import :Pass;
import :Validation;
import Runtime.RHI;

export namespace Vortex {

struct RDGGraph {
	std::vector<RDGPassNode> passes{};
	std::vector<std::unique_ptr<RDGTexture>> textures{};
	std::vector<std::unique_ptr<RDGBuffer>> buffers{};
	std::vector<std::unique_ptr<RDGTextureView>> texture_views{};
	std::vector<std::unique_ptr<RDGBufferView>> buffer_views{};
};


class RDGBuilder {
private:
	RDGGraph graph{};

	std::vector<RDGPassHandle> execution_order{};
	std::vector<RDGBarrier> epilogue_barriers{};

	bool compiled{};
	bool executed{};

	void buildDependencies();
	void calculateCulling();
	void calculateLifetimes();
	void buildBarrierPlan();
	void allocateResources(RHIDevice& device);

	static void addDependency(RDGPassNode& pass, RDGPassHandle dependency);
	static void applyBarriers(RHICommandList& command, std::span<const RDGBarrier> barriers);
	static void createPassViews(RHIDevice& device, const RDGPassNode& pass);

	RDGPassHandle addPass(RDGPassDesc desc,
	    std::function<void(RDGPassContext&)> execute);

public:
	RDGBuilder() = default;

	RDGBuilder(const RDGBuilder&) = delete;
	RDGBuilder& operator=(const RDGBuilder&) = delete;

	RDGBuilder(RDGBuilder&&) = delete;
	RDGBuilder& operator=(RDGBuilder&&) = delete;

	RDGTextureRef createTexture(std::string name, const RHITextureDesc& desc);
	RDGBufferRef createBuffer(std::string name, const RHIBufferDesc& desc);

	RDGTextureRef registerExternalTexture(std::string name, RHITexture& texture,
	    RHIResourceState initial_state, RHIResourceState final_state = Unknown);
	RDGBufferRef registerExternalBuffer(std::string name, RHIBuffer& buffer,
	    RHIResourceState initial_state, RHIResourceState final_state = Unknown);

	void addOutput(RDGTextureRef texture);
	void addOutput(RDGBufferRef buffer);

	void compile();
	void execute(RHIDevice& device, RHICommandList& command);

	template <typename Data, typename Setup, typename Execute>
	RDGPassHandle addPass(RDGPassDesc desc, Data data, Setup&& setup, Execute&& execute)
	{
		CHECK(!executed, "Cannot add a pass after the RDG has executed");
		validateRDGName(desc.name, "pass");
		if (desc.type == RDGPassType::Raster && !desc.render_targets)
			desc.render_targets.emplace();

		RDGPassBuilder builder(graph, desc);
		std::invoke(std::forward<Setup>(setup), builder, data);

		return addPass(std::move(desc),
		    [data = std::move(data),
		        execute = std::forward<Execute>(execute)](
		        RDGPassContext& context) mutable {
			    std::invoke(execute, context, data);
		    });
	}

	const std::vector<std::unique_ptr<RDGTexture>>& getTextures() const noexcept
	{
		return graph.textures;
	}

	const std::vector<std::unique_ptr<RDGBuffer>>& getBuffers() const noexcept
	{
		return graph.buffers;
	}

	const std::vector<RDGPassNode>& getPasses() const noexcept
	{
		return graph.passes;
	}
};

}        // namespace Vortex
