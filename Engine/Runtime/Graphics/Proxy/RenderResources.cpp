module;

#include <cassert>

module Runtime.Graphics;

namespace Vortex {

RenderMesh::RenderMesh(RHIContext& context, std::shared_ptr<SubMesh> submesh, RHIBindingLayout& layout) :
    RenderResource(context), submesh(submesh)
{
	assert(submesh && "Cannot create a render mesh with an empty submesh");

	auto command = getDevice().createCommandList(RHICommandListDesc{});
	command->open();

	if (submesh->getVerticesCount()) {
		RHIBufferDesc desc{};
		desc.setSize(submesh->getVerticesCount() * sizeof(Vertex))
		    .setStride(sizeof(Vertex))
		    .setUsage(RHIBufferUsage::VertexBuffer | RHIBufferUsage::CopyDest);

		vertex_buffer = getDevice().createBuffer(desc);
		command->writeBuffer(vertex_buffer.get(), 0, submesh->getVertices().data(), desc.size);
		command->transitionBuffer(vertex_buffer.get(), VertexBuffer);
	}

	if (submesh->getIndicesCount()) {
		RHIBufferDesc desc{};
		desc.setSize(submesh->getIndicesCount() * sizeof(uint32))
		    .setStride(sizeof(uint32))
		    .setUsage(RHIBufferUsage::IndexBuffer | RHIBufferUsage::CopyDest);

		index_buffer = getDevice().createBuffer(desc);
		command->writeBuffer(index_buffer.get(), 0, submesh->getIndices().data(), desc.size);
		command->transitionBuffer(index_buffer.get(), IndexBuffer);
	}

	command->close();
	getDevice().executeCommandList(command.get());

	RHIBufferDesc constant_buffer_desc{};
	constant_buffer_desc.setSize(sizeof(RenderObjectData))
	    .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest)
	    .setAccess(RHIAccessMode::Write);

	object_constant_buffer = getDevice().createBuffer(constant_buffer_desc);
	object_constant_buffer_view = getDevice().createBufferView(
	    RHIBufferViewDesc{}.setBuffer(object_constant_buffer.get()).setType(RHIBufferViewType::Constant));

	RHIBindingSetDesc set_desc{};
	set_desc.addItem(RHIBindingSetItem().setBufferView(0, object_constant_buffer_view.get()));
	object_binding_set = getDevice().createBindingSet(set_desc, layout);
}

void RenderMesh::updateGraphicsState(RHIGraphicsState& state) const
{
	if (object_binding_set)
		state.addBindingSet(object_binding_set.get());

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
	auto* mapped = getDevice().mapBuffer(object_constant_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, &object_data, sizeof(RenderObjectData));
	getDevice().unmapBuffer(object_constant_buffer.get());
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


RenderTexture::RenderTexture(RHIContext& context, std::shared_ptr<Texture> texture, RHIRef<RHISampler> default_sampler) :
    RenderResource(context), source_texture(texture)
{
	assert(texture && "Cannot create a render texture with an empty texture");

	RHITextureDesc texture_desc{};
	texture_desc.setWidth(texture->getWidth())
	    .setHeight(texture->getHeight())
	    .setFormat(RHIFormat::RGBA8_SRGB)
	    .setUsage(RHITextureUsage::Sampled | RHITextureUsage::CopyDest);
	image = getDevice().createTexture(texture_desc);
	sampled_view = getDevice().createTextureView(RHITextureViewDesc{}.setTexture(image.get()).setType(RHITextureViewType::ShaderResource));
	sampler = default_sampler ? default_sampler : getDevice().createSampler(RHISamplerDesc{});

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
    RHIBindingLayout&                      layout,
    RHITextureView*                        albedo,
    RHITextureView*                        metallic_roughness,
    RHISampler*                            sampler) :
    RenderResource(ctx),
    src_material(material)
{
	if (auto* mat = dynamic_cast<const Material*>(material.get())) {
		material_data.albedo = mat->getAlbedo();
		material_data.metallic = mat->getMetallic();
		material_data.roughness = mat->getRoughness();
	}

	RHIBufferDesc constant_buffer_desc{};
	constant_buffer_desc.setSize(sizeof(RenderMaterialData))
	    .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest)
	    .setAccess(RHIAccessMode::Write);
	material_constant_buffer = getDevice().createBuffer(constant_buffer_desc);
	material_constant_buffer_view = getDevice().createBufferView(
	    RHIBufferViewDesc{}.setBuffer(material_constant_buffer.get()).setType(RHIBufferViewType::Constant));

	RHIBindingSetDesc set_desc{};
	RHIBindingSetItem constant_buffer_item{};
	constant_buffer_item.setBufferView(0, material_constant_buffer_view.get());
	set_desc.addItem(constant_buffer_item);

	if (albedo) {
		RHIBindingSetItem albedo_item{};
		albedo_item.setTextureView(1, albedo);
		set_desc.addItem(albedo_item);
	}

	if (metallic_roughness) {
		RHIBindingSetItem metallic_roughness_item{};
		metallic_roughness_item.setTextureView(2, metallic_roughness);
		set_desc.addItem(metallic_roughness_item);
	}

	if (sampler) {
		RHIBindingSetItem sampler_item{};
		sampler_item.setSampler(3, sampler);
		set_desc.addItem(sampler_item);
	}

	material_binding_set = getDevice().createBindingSet(set_desc, layout);
	updateUniforms();
}

void RenderMaterial::updateGraphicsState(RHIGraphicsState& state) const
{
	if (material_binding_set)
		state.addBindingSet(material_binding_set.get());
}

void RenderMaterial::updateUniforms()
{
	void* mapped = getDevice().mapBuffer(material_constant_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, &material_data, sizeof(RenderMaterialData));
	getDevice().unmapBuffer(material_constant_buffer.get());
}

}        // namespace Vortex
