#pragma once

#include <memory>

#include "RenderData.hpp"
#include "Runtime/Render/RHI/RHICommand.hpp"
#include "Runtime/World/Resources/SubMesh.hpp"
#include "Runtime/World/Resources/Texture.hpp"

// Mesh
class RenderMesh : public RenderResource {
private:
	std::unique_ptr<RHIBuffer>        vertex_buffer;
	std::unique_ptr<RHIBuffer>        index_buffer;
	std::unique_ptr<RHIBuffer>        object_uniform;
	std::unique_ptr<RHIDescriptorSet> object_descriptor;

	RenderObjectData object_data;

	std::shared_ptr<SubMesh> submesh;

public:
	RenderMesh(RHIContext& context, std::shared_ptr<SubMesh> submesh, RHIDescriptorLayout& layout);

	void updateGraphicsState(RHIGraphicsState& state) const;
	void updateUniforms();
	void draw(RHICommandList& command) const;

	void setModelMatrix(const glm::mat4& model) { object_data.model = model; }

	RHIBuffer*        getVertexBuffer() const { return vertex_buffer.get(); }
	RHIBuffer*        getIndexBuffer() const { return index_buffer.get(); }
	RHIBuffer*        getUniformBuffer() const { return object_uniform.get(); }
	RHIDescriptorSet* getDescriptor() const { return object_descriptor.get(); }

	std::shared_ptr<SubMesh> getSrcSubMesh() const { return submesh; }
};


// Texture
class RenderTexture : public RenderResource {
private:
	std::unique_ptr<RHITexture> image;
	std::shared_ptr<RHISampler> sampler;

	std::shared_ptr<Texture> source_texture;

public:
	RenderTexture(RHIContext& context, std::shared_ptr<Texture> texture, std::shared_ptr<RHISampler> sampler = nullptr);

	RHITexture* getTexture() const { return image.get(); }
	RHISampler* getSampler() const { return sampler.get(); }

	std::shared_ptr<Texture> getSrcTexture() const { return source_texture; }
};


// Material
class RenderMaterial : public RenderResource {
private:
	std::unique_ptr<RHIDescriptorSet> material_descriptor;
	std::unique_ptr<RHIBuffer>        material_uniform;

	RenderMaterialData material_data;

	std::shared_ptr<Material> src_material;

	RHITexture* albedo{};
	RHITexture* metallic_roughness{};
	RHISampler* sampler{};

public:
	RenderMaterial(RHIContext&    context,
	    std::shared_ptr<Material> material,
	    RHIDescriptorLayout&      layout,
	    RHITexture*               albedo = {},
	    RHITexture*               metallic_roughness = {},
	    RHISampler*               sampler = {});

	void updateGraphicsState(RHIGraphicsState& state) const;
	void updateUniforms();

	RHIDescriptorSet* getDescriptor() const { return material_descriptor.get(); }

	std::shared_ptr<Material> getSrcMaterial() const { return src_material; }
};
