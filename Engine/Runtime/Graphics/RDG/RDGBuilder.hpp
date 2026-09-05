export module Runtime.Graphics:RDG.Builder;

import Core;
import :RDG.Pass;
import :RDG.Validation;
import :RHI.Device;

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

	std::vector<std::shared_ptr<void>> pass_parameters{};
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

	template <typename T, typename... Args>
	requires std::constructible_from<T, Args...>
	T* allocParameters(Args&&... args);

	RDGPassHandle addPass(std::unique_ptr<RDGPass> pass);

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

template <typename T, typename... Args>
requires std::constructible_from<T, Args...>
T* RDGBuilder::allocParameters(Args&&... args)
{
	CHECK(!executed, "Cannot allocate RDG pass parameters after execution");

	auto parameters = std::make_shared<T>(std::forward<Args>(args)...);
	auto* result = parameters.get();
	pass_parameters.emplace_back(std::move(parameters));
	return result;
}

}        // namespace Vortex
