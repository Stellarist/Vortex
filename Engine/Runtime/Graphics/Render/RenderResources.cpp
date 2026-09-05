module;

#include <cassert>

module Runtime.Graphics;

namespace Vortex {

RenderMesh::RenderMesh(RHIContext& context, AssetHandle<MeshAsset> mesh, RHIBindingLayout& layout) :
    RenderResource(context), source_mesh(std::move(mesh))
{
	assert(source_mesh && "Cannot create a render mesh with an empty mesh asset");

	auto command = getDevice().createCommandList(RHICommandListDesc{});
	command->open();

	if (source_mesh->getVertexCount()) {
		RHIBufferDesc desc{};
		desc.setSize(source_mesh->getVertexCount() * sizeof(Vertex))
		    .setStride(sizeof(Vertex))
		    .setUsage(RHIBufferUsage::VertexBuffer | RHIBufferUsage::CopyDest);

		vertex_buffer = getDevice().createBuffer(desc);
		command->writeBuffer(vertex_buffer.get(), 0, source_mesh->getVertices().data(), desc.size);
		command->transitionBuffer(vertex_buffer.get(), VertexBuffer);
	}

	if (source_mesh->getIndexCount()) {
		RHIBufferDesc desc{};
		desc.setSize(source_mesh->getIndexCount() * sizeof(uint32))
		    .setStride(sizeof(uint32))
		    .setUsage(RHIBufferUsage::IndexBuffer | RHIBufferUsage::CopyDest);

		index_buffer = getDevice().createBuffer(desc);
		command->writeBuffer(index_buffer.get(), 0, source_mesh->getIndices().data(), desc.size);
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

void RenderMesh::draw(RHICommandList& command, const MeshSection& section) const
{
	RHIDrawArguments args{};
	args.setVertexCount(section.index_count)
	    .setInstanceCount(1)
	    .setStartIndex(section.first_index)
	    .setStartVertex(section.first_vertex)
	    .setStartInstance(0);

	command.drawIndexed(args);
}


RenderTexture::RenderTexture(RHIContext& context, AssetHandle<TextureAsset> texture, RHIRef<RHISampler> default_sampler) :
    RenderResource(context), source_texture(std::move(texture))
{
	assert(source_texture && "Cannot create a render texture with an empty texture asset");

	RHITextureDesc texture_desc{};
	texture_desc.setWidth(source_texture->getWidth())
	    .setHeight(source_texture->getHeight())
	    .setFormat(RHIFormat::RGBA8_SRGB)
	    .setUsage(RHITextureUsage::Sampled | RHITextureUsage::CopyDest);
	image = getDevice().createTexture(texture_desc);
	sampled_view = getDevice().createTextureView(RHITextureViewDesc{}.setTexture(image.get()).setType(RHITextureViewType::ShaderResource));
	sampler = default_sampler ? default_sampler : getDevice().createSampler(RHISamplerDesc{});

	auto command = getDevice().createCommandList(RHICommandListDesc{});
	command->open();

	RHITextureSlice slice{};
	slice.setExtent(static_cast<int>(source_texture->getWidth()),
	    static_cast<int>(source_texture->getHeight()),
	    1);

	command->writeTexture(image.get(), slice, source_texture->getData().data(), source_texture->getData().size());
	command->transitionTexture(image.get(), ShaderResource);
	command->close();

	getDevice().executeCommandList(command.get());
}


RenderMaterial::RenderMaterial(RHIContext& ctx,
    AssetHandle<MaterialAsset> material,
    RHIBindingLayout& layout,
    RHITextureView* albedo,
    RHITextureView* metallic_roughness,
    RHISampler* sampler) :
    RenderResource(ctx),
    source_material(std::move(material))
{
	if (source_material) {
		material_data.albedo = source_material->getAlbedo();
		material_data.metallic = source_material->getMetallic();
		material_data.roughness = source_material->getRoughness();
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
