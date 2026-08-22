#pragma once

#include <memory>

#include "RenderData.hpp"
#include "Runtime/Render/RHI/RHICommand.hpp"
#include "Runtime/World/Resources/SubMesh.hpp"
#include "Runtime/World/Resources/Texture.hpp"

// Mesh
class RenderMesh : public RenderResource {
private:
	RHIRef<RHIBuffer>     vertex_buffer;
	RHIRef<RHIBuffer>     index_buffer;
	RHIRef<RHIBuffer>     object_constant_buffer;
	RHIRef<RHIBufferView> object_constant_buffer_view;
	RHIRef<RHIBindingSet> object_binding_set;

	RenderObjectData object_data;

	std::shared_ptr<SubMesh> submesh;

public:
	RenderMesh(RHIContext& context, std::shared_ptr<SubMesh> submesh, RHIBindingLayout& layout);

	void updateGraphicsState(RHIGraphicsState& state) const;
	void updateUniforms();
	void draw(RHICommandList& command) const;

	void setModelMatrix(const glm::mat4& model) { object_data.model = model; }

	RHIBuffer*     getVertexBuffer() const { return vertex_buffer.get(); }
	RHIBuffer*     getIndexBuffer() const { return index_buffer.get(); }
	RHIBuffer*     getConstantBuffer() const { return object_constant_buffer.get(); }
	RHIBindingSet* getBindingSet() const { return object_binding_set.get(); }

	std::shared_ptr<SubMesh> getSrcSubMesh() const { return submesh; }
};


// Texture
class RenderTexture : public RenderResource {
private:
	RHIRef<RHITexture>     image;
	RHIRef<RHITextureView> sampled_view;
	RHIRef<RHISampler>     sampler;

	std::shared_ptr<Texture> source_texture;

public:
	RenderTexture(RHIContext& context, std::shared_ptr<Texture> texture, RHIRef<RHISampler> sampler = nullptr);

	RHITexture*     getTexture() const { return image.get(); }
	RHITextureView* getTextureView() const { return sampled_view.get(); }
	RHISampler*     getSampler() const { return sampler.get(); }

	std::shared_ptr<Texture> getSrcTexture() const { return source_texture; }
};


// Material
class RenderMaterial : public RenderResource {
private:
	RHIRef<RHIBuffer>     material_constant_buffer;
	RHIRef<RHIBufferView> material_constant_buffer_view;
	RHIRef<RHIBindingSet> material_binding_set;

	RenderMaterialData material_data;

	std::shared_ptr<Material> src_material;

public:
	RenderMaterial(RHIContext&    context,
	    std::shared_ptr<Material> material,
	    RHIBindingLayout&         layout,
	    RHITextureView*           albedo = {},
	    RHITextureView*           metallic_roughness = {},
	    RHISampler*               sampler = {});

	void updateGraphicsState(RHIGraphicsState& state) const;
	void updateUniforms();

	RHIBindingSet* getBindingSet() const { return material_binding_set.get(); }

	std::shared_ptr<Material> getSrcMaterial() const { return src_material; }
};
