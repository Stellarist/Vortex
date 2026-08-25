export module Runtime.Graphics:Render.Resources;

import Core;
import Runtime.World;
import :Render.Data;
import :RHI.Command;

export namespace Vortex {

// Mesh
class RenderMesh : public RenderResource {
private:
	RHIRef<RHIBuffer>     vertex_buffer;
	RHIRef<RHIBuffer>     index_buffer;
	RHIRef<RHIBuffer>     object_constant_buffer;
	RHIRef<RHIBufferView> object_constant_buffer_view;
	RHIRef<RHIBindingSet> object_binding_set;

	RenderObjectData object_data;

	AssetHandle<MeshAsset> source_mesh;

public:
	RenderMesh(RHIContext& context, AssetHandle<MeshAsset> mesh, RHIBindingLayout& layout);

	void updateGraphicsState(RHIGraphicsState& state) const;
	void updateUniforms();
	void draw(RHICommandList& command, const MeshSection& section) const;

	void setModelMatrix(const Mat4& model) { object_data.model = model; }

	RHIBuffer*     getVertexBuffer() const { return vertex_buffer.get(); }
	RHIBuffer*     getIndexBuffer() const { return index_buffer.get(); }
	RHIBuffer*     getConstantBuffer() const { return object_constant_buffer.get(); }
	RHIBindingSet* getBindingSet() const { return object_binding_set.get(); }

	const AssetHandle<MeshAsset>& getSourceMesh() const noexcept { return source_mesh; }
};


// Texture
class RenderTexture : public RenderResource {
private:
	RHIRef<RHITexture>     image;
	RHIRef<RHITextureView> sampled_view;
	RHIRef<RHISampler>     sampler;

	AssetHandle<TextureAsset> source_texture;

public:
	RenderTexture(RHIContext& context, AssetHandle<TextureAsset> texture, RHIRef<RHISampler> sampler = nullptr);

	RHITexture*     getTexture() const { return image.get(); }
	RHITextureView* getTextureView() const { return sampled_view.get(); }
	RHISampler*     getSampler() const { return sampler.get(); }

	const AssetHandle<TextureAsset>& getSourceTexture() const noexcept { return source_texture; }
};


// Material
class RenderMaterial : public RenderResource {
private:
	RHIRef<RHIBuffer>     material_constant_buffer;
	RHIRef<RHIBufferView> material_constant_buffer_view;
	RHIRef<RHIBindingSet> material_binding_set;

	RenderMaterialData material_data;

	AssetHandle<MaterialAsset> source_material;

public:
	RenderMaterial(RHIContext&     context,
	    AssetHandle<MaterialAsset> material,
	    RHIBindingLayout&          layout,
	    RHITextureView*            albedo = {},
	    RHITextureView*            metallic_roughness = {},
	    RHISampler*                sampler = {});

	void updateGraphicsState(RHIGraphicsState& state) const;
	void updateUniforms();

	RHIBindingSet* getBindingSet() const { return material_binding_set.get(); }

	const AssetHandle<MaterialAsset>& getSourceMaterial() const noexcept { return source_material; }
};

}        // namespace Vortex
