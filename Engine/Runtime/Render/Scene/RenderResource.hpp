export module Runtime.Render:Scene.Resource;

import Core;
import Runtime.World;
import Runtime.RHI;

export namespace Vortex {

template <typename Asset>
class RenderResource {
private:
	AssetHandle<Asset> source;
	uint64 source_revision{};

protected:
	RenderResource(AssetHandle<Asset> asset) :
	    source(std::move(asset)),
	    source_revision(source ? source->getRevision() : 0)
	{}

public:
	const AssetHandle<Asset>& getSource() const noexcept { return source; }
	uint64 getSourceRevision() const noexcept { return source_revision; }
};


class MeshResource final : public RenderResource<MeshAsset> {
private:
	RHIRef<RHIBuffer> vertex_buffer;
	RHIRef<RHIBuffer> index_buffer;

public:
	MeshResource(RHIDevice& device, AssetHandle<MeshAsset> mesh,
	    RHICommandList* upload_command = nullptr);

	static RHIVertexBindingDesc vertexBinding(uint32 binding = 0);
	static std::vector<RHIVertexAttributeDesc> vertexAttributes(uint32 binding = 0);

	RHIBuffer* getVertexBuffer() const { return vertex_buffer.get(); }
	RHIBuffer* getIndexBuffer() const { return index_buffer.get(); }
};


class TextureResource final : public RenderResource<TextureAsset> {
private:
	RHIRef<RHITexture> image;
	RHIRef<RHITextureView> sampled_view;
	RHIRef<RHISampler> sampler;

public:
	TextureResource(RHIDevice& device, AssetHandle<TextureAsset> texture,
	    RHIRef<RHISampler> sampler = nullptr,
	    RHICommandList* upload_command = nullptr);

	RHITexture* getTexture() const { return image.get(); }
	RHITextureView* getTextureView() const { return sampled_view.get(); }
	RHISampler* getSampler() const { return sampler.get(); }
};


class MaterialResource final : public RenderResource<MaterialAsset> {
private:
	RHIRef<RHIBuffer> material_constant_buffer;
	RHIRef<RHIBufferView> material_constant_buffer_view;
	RHIRef<RHIBindingSet> material_binding_set;

	void updateUniforms(RHIDevice& device);

public:
	MaterialResource(RHIDevice& device, AssetHandle<MaterialAsset> material,
	    RHIBindingLayout& layout, RHITextureView* albedo = {},
	    RHITextureView* metallic_roughness = {}, RHISampler* sampler = {});

	RHIBindingSet* getBindingSet() const { return material_binding_set.get(); }
};


class RenderResourceCache {
private:
	RHIDevice* device{};
	RHIRef<RHIBindingLayout> material_layout;
	RHIRef<RHIBindingLayout> object_layout;

	RHIRef<RHISampler> material_sampler;
	RHIRef<RHITexture> fallback_texture;
	RHIRef<RHITextureView> fallback_texture_view;

	std::unordered_map<uint64, std::unique_ptr<TextureResource>> textures;
	std::unordered_map<uint64, std::unique_ptr<MaterialResource>> materials;
	std::unordered_map<uint64, std::unique_ptr<MeshResource>> meshes;

	void ensureFallbackResources(RHICommandList& upload_command);

public:
	RenderResourceCache(RHIDevice& device, RHIBindingLayout& material_layout,
	    RHIBindingLayout& object_layout);

	void synchronize(const World& world);

	MeshResource* findMesh(const AssetHandle<MeshAsset>& asset) const;
	MaterialResource* findMaterial(const AssetHandle<MaterialAsset>& asset) const;
	TextureResource* findTexture(const AssetHandle<TextureAsset>& asset) const;

	RHIBindingLayout* getMaterialLayout() const noexcept { return material_layout.get(); }
	RHIBindingLayout* getObjectLayout() const noexcept { return object_layout.get(); }

	size_t getMeshCount() const noexcept { return meshes.size(); }
	size_t getMaterialCount() const noexcept { return materials.size(); }
	size_t getTextureCount() const noexcept { return textures.size(); }
};

}        // namespace Vortex
