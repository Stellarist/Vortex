#include "RenderResources.hpp"

#include <cstring>

#include "Runtime/Render/RHI/RHIDevice.hpp"

RenderMesh::RenderMesh(RHIContext& context, std::shared_ptr<SubMesh> submesh, RHIDescriptorLayout& layout) :
    RenderResource(context), submesh(submesh)
{
	assert(submesh && "Cannot create a render mesh with an empty submesh");

	auto command = getDevice().createCommandList(RHICommandListDesc{});
	command->open();

	if (submesh->getVerticesCount()) {
		RHIBufferDesc desc{};
		desc.setSize(submesh->getVerticesCount() * sizeof(Vertex))
		    .setStride(sizeof(Vertex))
		    .setUsage(RHIBufferUsage::VertexBuffer | RHIBufferUsage::CopyDst);

		vertex_buffer = getDevice().createBuffer(desc);
		command->writeBuffer(vertex_buffer.get(), 0, submesh->getVertices().data(), desc.size);
		command->transitionBuffer(vertex_buffer.get(), VertexBuffer);
	}

	if (submesh->getIndicesCount()) {
		RHIBufferDesc desc{};
		desc.setSize(submesh->getIndicesCount() * sizeof(uint32_t))
		    .setStride(sizeof(uint32_t))
		    .setUsage(RHIBufferUsage::IndexBuffer | RHIBufferUsage::CopyDst);

		index_buffer = getDevice().createBuffer(desc);
		command->writeBuffer(index_buffer.get(), 0, submesh->getIndices().data(), desc.size);
		command->transitionBuffer(index_buffer.get(), IndexBuffer);
	}

	command->close();
	getDevice().executeCommandList(command.get());

	RHIBufferDesc uniform_desc{};
	uniform_desc.setSize(sizeof(RenderObjectData))
	    .setUsage(RHIBufferUsage::UniformBuffer | RHIBufferUsage::CopyDst)
	    .setAccess(RHIAccessMode::Write);
	object_uniform = getDevice().createBuffer(uniform_desc);

	RHIDescriptorSetDesc set_desc{};
	set_desc.addItem(RHIDescriptorSetItem()
	        .setBuffer(0, object_uniform.get()));
	object_descriptor = getDevice().createDescriptorSet(set_desc, layout);
}

void RenderMesh::updateGraphicsState(RHIGraphicsState& state) const
{
	if (object_descriptor)
		state.addBindingSet(object_descriptor.get());

	if (vertex_buffer)
		state.addVertexBuffer(RHIVertexBufferBinding()
		        .setBuffer(vertex_buffer.get())
		        .setSlot(0)
		        .setOffset(0));

	if (index_buffer)
		state.setIndexBuffer(RHIIndexBufferBinding()
		        .setBuffer(index_buffer.get())
		        .setIndexType(RHIIndexType::Uint32)
		        .setOffset(0));
}

void RenderMesh::updateUniforms()
{
	auto* mapped = getDevice().mapBuffer(object_uniform.get(), RHIAccessMode::Write);
	std::memcpy(mapped, &object_data, sizeof(RenderObjectData));
	getDevice().unmapBuffer(object_uniform.get());
}

void RenderMesh::draw(RHICommandList& command) const
{
	RHIDrawArguments args{};
	args.setVertexCount(submesh->getIndicesCount())
	    .setInstanceCount(1)
	    .setStartIndex(0)
	    .setStartVertex(0)
	    .setStartInstance(0);

	command.drawIndexed(args);
}


RenderTexture::RenderTexture(RHIContext& context, std::shared_ptr<Texture> texture, std::shared_ptr<RHISampler> default_sampler) :
    RenderResource(context), source_texture(texture)
{
	assert(texture && "Cannot create a render texture with an empty texture");

	RHITextureDesc texture_desc{};
	texture_desc.setWidth(texture->getWidth())
	    .setHeight(texture->getHeight())
	    .setFormat(RHIFormat::RGBA8_SRGB)
	    .setUsage(RHITextureUsage::Sampled | RHITextureUsage::CopyDst);
	image = getDevice().createTexture(texture_desc);
	sampler = default_sampler ? default_sampler : std::move(getDevice().createSampler(RHISamplerDesc{}));

	auto command = getDevice().createCommandList(RHICommandListDesc{});
	command->open();

	RHITextureSlice slice{};
	slice.setExtent(static_cast<int>(texture->getWidth()),
	    static_cast<int>(texture->getHeight()),
	    1);

	command->writeTexture(image.get(), slice, texture->getData().data(), texture->getData().size());
	command->transitionTexture(image.get(), ShaderResource);
	command->close();

	getDevice().executeCommandList(command.get());
}


RenderMaterial::RenderMaterial(RHIContext& ctx,
    std::shared_ptr<Material>              material,
    RHIDescriptorLayout&                   layout,
    RHITexture*                            albedo,
    RHITexture*                            metallic_roughness,
    RHISampler*                            sampler) :
    RenderResource(ctx),
    src_material(material),
    albedo(albedo),
    metallic_roughness(metallic_roughness),
    sampler(sampler)
{
	if (auto* mat = dynamic_cast<const Material*>(material.get())) {
		material_data.albedo = mat->getAlbedo();
		material_data.metallic = mat->getMetallic();
		material_data.roughness = mat->getRoughness();
	}

	RHIBufferDesc uniform_desc{};
	uniform_desc.setSize(sizeof(RenderMaterialData))
	    .setUsage(RHIBufferUsage::UniformBuffer | RHIBufferUsage::CopyDst)
	    .setAccess(RHIAccessMode::Write);
	material_uniform = getDevice().createBuffer(uniform_desc);

	RHIDescriptorSetDesc set_desc{};
	RHIDescriptorSetItem mat_uniform{};
	mat_uniform.setBuffer(0, material_uniform.get());
	set_desc.addItem(mat_uniform);

	if (albedo) {
		RHIDescriptorSetItem albedo_item{};
		albedo_item.setTexture(1, albedo);
		set_desc.addItem(albedo_item);
	}

	if (metallic_roughness) {
		RHIDescriptorSetItem metallic_roughness_item{};
		metallic_roughness_item.setTexture(2, metallic_roughness);
		set_desc.addItem(metallic_roughness_item);
	}

	if (sampler) {
		RHIDescriptorSetItem sampler_item{};
		sampler_item.setSampler(3, sampler);
		set_desc.addItem(sampler_item);
	}

	material_descriptor = getDevice().createDescriptorSet(set_desc, layout);
	updateUniforms();
}

void RenderMaterial::updateGraphicsState(RHIGraphicsState& state) const
{
	if (material_descriptor)
		state.addBindingSet(material_descriptor.get());
}

void RenderMaterial::updateUniforms()
{
	void* mapped = getDevice().mapBuffer(material_uniform.get(), RHIAccessMode::Write);
	std::memcpy(mapped, &material_data, sizeof(RenderMaterialData));
	getDevice().unmapBuffer(material_uniform.get());
}
