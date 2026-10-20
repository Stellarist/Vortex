export module Runtime.Render:Manager;

import Core;
import Runtime.RHI;

namespace Vortex {

struct RenderScene;

struct RenderMesh {
	uint64 revision{};
	RHIRef<RHIBuffer> vertices{};
	RHIRef<RHIBuffer> indices{};
};

struct RenderTexture {
	uint64 revision{};
	RHIRef<RHITexture> image{};
	RHIRef<RHITextureView> view{};
};

struct RenderMaterial {
	uint64 revision{};
	uint64 texture_epoch{};
	RHIRef<RHIBuffer> constants{};
	RHIRef<RHIBufferView> constant_view{};
	RHIRef<RHIBindingSet> binding{};
};

class RenderManager {
private:
	struct ShaderEntry {
		std::string file{};
		RHIRef<RHIShader> shader{};
	};

	struct InputEntry {
		RHIInputLayoutDesc desc{};
		RHIRef<RHIInputLayout> layout{};
	};

	RHIDevice* device{};
	RHIRef<RHIBindingLayout> material_layout{};
	RHIRef<RHISampler> material_sampler{};
	RHIRef<RHITexture> fallback_image{};
	RHIRef<RHITextureView> fallback_view{};
	std::unordered_map<uint64, RenderMesh> meshes{};
	std::unordered_map<uint64, RenderTexture> textures{};
	std::unordered_map<uint64, RenderMaterial> materials{};
	uint64 texture_epoch{};

	std::vector<ShaderEntry> shaders{};
	std::vector<InputEntry> inputs{};
	std::vector<RHIRef<RHIBindingLayout>> layouts{};
	std::vector<RHIRef<RHISampler>> samplers{};
	std::vector<RHIRef<RHIGraphicsPipeline>> graphics_pipelines{};
	std::vector<RHIRef<RHIComputePipeline>> compute_pipelines{};

public:
	RenderManager(RHIDevice& device);
	void synchronize(const RenderScene& scene, RHICommandList& command);
	const RenderMesh* findMesh(uint64 id) const noexcept;
	const RenderMaterial* findMaterial(uint64 id) const noexcept;

	RHIDevice& getDevice() const noexcept { return *device; }
	RHIBindingLayout& materialLayout() const noexcept { return *material_layout; }
	RHIRef<RHIBuffer> uploadBuffer(std::string name, const RHIBufferDesc& desc,
	    std::span<const std::byte> bytes);
	RHIShader& shader(std::string_view file, RHIShaderType stage, std::string_view entry);
	RHIInputLayout& inputLayout(const RHIInputLayoutDesc& desc);
	RHIBindingLayout& bindingLayout(const RHIBindingLayoutDesc& desc);
	RHISampler& sampler(const RHISamplerDesc& desc);
	RHIGraphicsPipeline& graphicsPipeline(const RHIGraphicsPipelineDesc& desc);
	RHIComputePipeline& computePipeline(const RHIComputePipelineDesc& desc);
};

}        // namespace Vortex
