module;

#include <cstddef>

module Runtime.Render;

namespace Vortex {

namespace {

struct MaterialConstants {
	Vec4 albedo{1.0f};
	float metallic{};
	float roughness{1.0f};
	float alpha_cutoff{0.5f};
	uint32 alpha_mode{};
	Vec4 emissive_ao{0.0f, 0.0f, 0.0f, 1.0f};
};

static_assert(sizeof(MeshVertex) == 48);
static_assert(sizeof(MaterialConstants) == 48);

}        // namespace

RHIVertexBindingDesc MeshResource::vertexBinding(uint32 binding)
{
	return {binding, sizeof(MeshVertex), false};
}

std::vector<RHIVertexAttributeDesc> MeshResource::vertexAttributes(uint32 binding)
{
	return {
	    {"POSITION", RHIFormat::RGB32_FLOAT, 0, binding, offsetof(MeshVertex, pos)},
	    {"NORMAL", RHIFormat::RGB32_FLOAT, 1, binding, offsetof(MeshVertex, normal)},
	    {"TEXCOORD", RHIFormat::RG32_FLOAT, 2, binding, offsetof(MeshVertex, uv)},
	    {"COLOR", RHIFormat::RGBA32_FLOAT, 3, binding, offsetof(MeshVertex, color)},
	};
}

MeshResource::MeshResource(RHIDevice& device, AssetHandle<MeshAsset> mesh,
    RHICommandList* upload_command) :
    RenderResource(std::move(mesh))
{
	const auto& source = getSource();
	CHECK(source && source->valid(),
	    "Cannot create a render mesh from an empty or invalid mesh asset");

	RHIRef<RHICommandList> owned_command;
	if (!upload_command) {
		owned_command = device.createCommandList(RHICommandListDesc{});
		upload_command = owned_command.get();
		upload_command->open();
	}

	if (source->getVertexCount()) {
		const auto& vertices = source->getVertices();
		RHIBufferDesc desc{};
		desc.setSize(vertices.size() * sizeof(MeshVertex))
		    .setStride(sizeof(MeshVertex))
		    .setUsage(RHIBufferUsage::VertexBuffer | RHIBufferUsage::CopyDest);
		vertex_buffer = device.createBuffer(desc);
		vertex_buffer->setName(std::format("{}.Vertices", source->getName()));
		upload_command->writeBuffer(vertex_buffer.get(), 0, vertices.data(), desc.size);
		upload_command->transitionBuffer(vertex_buffer.get(), CopyDest, VertexBuffer);
	}

	if (source->getIndexCount()) {
		RHIBufferDesc desc{};
		desc.setSize(source->getIndexCount() * sizeof(uint32))
		    .setStride(sizeof(uint32))
		    .setUsage(RHIBufferUsage::IndexBuffer | RHIBufferUsage::CopyDest);
		index_buffer = device.createBuffer(desc);
		index_buffer->setName(std::format("{}.Indices", source->getName()));
		upload_command->writeBuffer(index_buffer.get(), 0,
		    source->getIndices().data(), desc.size);
		upload_command->transitionBuffer(index_buffer.get(), CopyDest, IndexBuffer);
	}

	if (owned_command) {
		upload_command->close();
		device.executeCommandList(upload_command);
	}
}

TextureResource::TextureResource(RHIDevice& device,
    AssetHandle<TextureAsset> texture, RHIRef<RHISampler> default_sampler,
    RHICommandList* upload_command) :
    RenderResource(std::move(texture))
{
	const auto& source = getSource();
	CHECK(source && source->valid(),
	    "Cannot create a render texture from an empty or invalid texture asset");

	RHITextureDesc texture_desc{};
	texture_desc.setWidth(source->getWidth())
	    .setHeight(source->getHeight())
	    .setFormat(RHIFormat::RGBA8_SRGB)
	    .setUsage(RHITextureUsage::Sampled | RHITextureUsage::CopyDest);
	image = device.createTexture(texture_desc);
	image->setName(source->getName());
	sampled_view = device.createTextureView(
	    RHITextureViewDesc{}.setTexture(image.get()).setType(RHITextureViewType::ShaderResource));
	sampled_view->setName(std::format("{}.SRV", source->getName()));
	sampler = default_sampler ? default_sampler : device.createSampler(RHISamplerDesc{});

	RHIRef<RHICommandList> owned_command;
	if (!upload_command) {
		owned_command = device.createCommandList(RHICommandListDesc{});
		upload_command = owned_command.get();
		upload_command->open();
	}

	RHITextureSlice slice{};
	slice.setExtent(static_cast<int>(source->getWidth()),
	    static_cast<int>(source->getHeight()), 1);
	upload_command->writeTexture(image.get(), slice,
	    source->getData().data(), source->getData().size());
	upload_command->transitionTexture(image.get(), CopyDest, ShaderResource);

	if (owned_command) {
		upload_command->close();
		device.executeCommandList(upload_command);
	}
}

MaterialResource::MaterialResource(RHIDevice& device,
    AssetHandle<MaterialAsset> material, RHIBindingLayout& layout,
    RHITextureView* albedo, RHITextureView* metallic_roughness,
    RHISampler* sampler) :
    RenderResource(std::move(material))
{
	const auto& source = getSource();
	CHECK(source, "Cannot create a render material from an empty material asset");
	const std::string debug_name = source->getName();

	RHIBufferDesc constant_buffer_desc{};
	constant_buffer_desc.setSize(sizeof(MaterialConstants))
	    .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest)
	    .setAccess(RHIAccessMode::Write);
	material_constant_buffer = device.createBuffer(constant_buffer_desc);
	material_constant_buffer->setName(std::format("{}.Constants", debug_name));
	material_constant_buffer_view = device.createBufferView(
	    RHIBufferViewDesc{}.setBuffer(material_constant_buffer.get()).setType(RHIBufferViewType::Constant));
	material_constant_buffer_view->setName(std::format("{}.Constants.CBV", debug_name));

	RHIBindingSetDesc set_desc{};
	set_desc.addItem(RHIBindingSetItem::constantBuffer(0, material_constant_buffer_view.get()));
	if (albedo)
		set_desc.addItem(RHIBindingSetItem::textureSRV(1, albedo));
	if (metallic_roughness)
		set_desc.addItem(RHIBindingSetItem::textureSRV(2, metallic_roughness));
	if (sampler)
		set_desc.addItem(RHIBindingSetItem::sampler(3, sampler));
	material_binding_set = device.createBindingSet(set_desc, layout);
	material_binding_set->setName(std::format("{}.BindingSet", debug_name));
	updateUniforms(device);
}

void MaterialResource::updateUniforms(RHIDevice& device)
{
	const auto& source = getSource();
	MaterialConstants data{};
	data.albedo = source->getAlbedo();
	data.metallic = source->getMetallic();
	data.roughness = source->getRoughness();
	data.alpha_cutoff = source->getAlphaCutoff();
	data.alpha_mode = static_cast<uint32>(source->getAlphaMode());
	data.emissive_ao = Vec4(source->getEmissive(), 1.0f);

	void* mapped = device.mapBuffer(material_constant_buffer.get(), RHIAccessMode::Write);
	CHECK(mapped, "Failed to map the material constant buffer");
	std::memcpy(mapped, &data, sizeof(data));
	device.unmapBuffer(material_constant_buffer.get());
}

RenderResourceCache::RenderResourceCache(RHIDevice& new_device,
    RHIBindingLayout& new_material_layout, RHIBindingLayout& new_object_layout) :
    device(&new_device),
    material_layout(&new_material_layout),
    object_layout(&new_object_layout)
{
	material_sampler = device->createSampler(RHISamplerDesc{});
	material_sampler->setName("MaterialSampler");
}

void RenderResourceCache::ensureFallbackResources(RHICommandList& upload_command)
{
	if (fallback_texture)
		return;

	fallback_texture = device->createTexture(RHITextureDesc{}
	        .setWidth(1)
	        .setHeight(1)
	        .setFormat(RHIFormat::RGBA8_SRGB)
	        .setUsage(RHITextureUsage::Sampled | RHITextureUsage::CopyDest));
	fallback_texture->setName("FallbackWhiteTexture");
	fallback_texture_view = device->createTextureView(RHITextureViewDesc{}
	        .setTexture(fallback_texture.get())
	        .setType(RHITextureViewType::ShaderResource));
	fallback_texture_view->setName("FallbackWhiteTexture.SRV");

	constexpr std::array<uint8, 4> white{255, 255, 255, 255};
	upload_command.writeTexture(fallback_texture.get(),
	    RHITextureSlice{}.setExtent(1, 1), white.data(), white.size());
	upload_command.transitionTexture(fallback_texture.get(), CopyDest, ShaderResource);
}

void RenderResourceCache::synchronize(const World& world)
{
	std::unordered_map<uint64, AssetHandle<MeshAsset>> mesh_assets;
	std::unordered_map<uint64, AssetHandle<MaterialAsset>> material_assets;
	std::unordered_map<uint64, AssetHandle<TextureAsset>> texture_assets;

	for (const auto* component : world.getComponents<MeshComponent>()) {
		if (!component || !component->getMesh() || !component->getMesh()->valid())
			continue;
		const auto mesh = component->getMesh();
		mesh_assets.insert_or_assign(mesh->getUid(), mesh);
		for (const auto& section : mesh->getSections()) {
			const auto material = component->getMaterial(section.material_slot);
			if (!material)
				continue;
			material_assets.insert_or_assign(material->getUid(), material);
			for (const auto& texture : material->getTextures())
				if (texture && texture->valid())
					texture_assets.insert_or_assign(texture->getUid(), texture);
		}
	}

	bool requires_upload = !fallback_texture;
	for (const auto& [id, asset] : texture_assets) {
		const auto it = textures.find(id);
		requires_upload |= it == textures.end() ||
		    it->second->getSourceRevision() != asset->getRevision();
	}
	for (const auto& [id, asset] : mesh_assets) {
		const auto it = meshes.find(id);
		requires_upload |= it == meshes.end() ||
		    it->second->getSourceRevision() != asset->getRevision();
	}

	RHIRef<RHICommandList> upload_command;
	if (requires_upload) {
		upload_command = device->createCommandList(RHICommandListDesc{});
		upload_command->open();
		ensureFallbackResources(*upload_command);
	}

	bool textures_changed = false;
	for (const auto& [id, asset] : texture_assets) {
		const auto it = textures.find(id);
		if (it != textures.end() &&
		    it->second->getSourceRevision() == asset->getRevision())
			continue;
		CHECK(upload_command, "Texture uploads require an upload command list");
		textures.insert_or_assign(id, std::make_unique<TextureResource>(*device, asset, material_sampler, upload_command.get()));
		textures_changed = true;
	}

	for (const auto& [id, asset] : mesh_assets) {
		const auto it = meshes.find(id);
		if (it != meshes.end() &&
		    it->second->getSourceRevision() == asset->getRevision())
			continue;
		CHECK(upload_command, "Mesh uploads require an upload command list");
		meshes.insert_or_assign(id, std::make_unique<MeshResource>(*device, asset, upload_command.get()));
	}

	if (upload_command) {
		upload_command->close();
		device->executeCommandList(upload_command.get());
	}

	for (const auto& [id, asset] : material_assets) {
		const auto it = materials.find(id);
		if (!textures_changed && it != materials.end() &&
		    it->second->getSourceRevision() == asset->getRevision())
			continue;

		RHITextureView* albedo = fallback_texture_view.get();
		RHITextureView* metallic_roughness = fallback_texture_view.get();
		RHISampler* sampler = material_sampler.get();
		if (const auto texture = asset->getTexture(MaterialTextureSlot::BaseColor); texture) {
			const auto texture_it = textures.find(texture->getUid());
			if (texture_it != textures.end()) {
				albedo = texture_it->second->getTextureView();
				sampler = texture_it->second->getSampler();
			}
		}
		if (const auto texture = asset->getTexture(MaterialTextureSlot::MetallicRoughness); texture) {
			const auto texture_it = textures.find(texture->getUid());
			if (texture_it != textures.end())
				metallic_roughness = texture_it->second->getTextureView();
		}
		materials.insert_or_assign(id, std::make_unique<MaterialResource>(*device, asset, *material_layout, albedo, metallic_roughness, sampler));
	}
}

MeshResource* RenderResourceCache::findMesh(const AssetHandle<MeshAsset>& asset) const
{
	if (!asset)
		return nullptr;
	const auto it = meshes.find(asset->getUid());
	return it != meshes.end() ? it->second.get() : nullptr;
}

MaterialResource* RenderResourceCache::findMaterial(
    const AssetHandle<MaterialAsset>& asset) const
{
	if (!asset)
		return nullptr;
	const auto it = materials.find(asset->getUid());
	return it != materials.end() ? it->second.get() : nullptr;
}

TextureResource* RenderResourceCache::findTexture(
    const AssetHandle<TextureAsset>& asset) const
{
	if (!asset)
		return nullptr;
	const auto it = textures.find(asset->getUid());
	return it != textures.end() ? it->second.get() : nullptr;
}

}        // namespace Vortex
