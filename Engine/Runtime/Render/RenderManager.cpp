module;
#include <cstddef>
module Runtime.Render;

namespace Vortex {

struct RenderMaterialConstants {
	Vec4 base_color{1.0f};
	float metallic{};
	float roughness{1.0f};
	float alpha_cutoff{0.5f};
	uint32 alpha_mode{};
	Vec4 emissive_ao{0.0f, 0.0f, 0.0f, 1.0f};
};
static_assert(sizeof(MeshVertex) == 48);
static_assert(sizeof(RenderMaterialConstants) == 48);

RenderManager::RenderManager(RHIDevice& new_device) :
    device(&new_device)
{
	material_layout = &bindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::textureSRV(1))
	        .addItem(RHIBindingLayoutItem::textureSRV(2))
	        .addItem(RHIBindingLayoutItem::sampler(3)));
	material_sampler = &sampler(RHISamplerDesc{});
	material_sampler->setName("Render.MaterialSampler");
}

void RenderManager::synchronize(const RenderScene& scene, RHICommandList& command)
{
	if (!fallback_image) {
		fallback_image = device->createTexture(RHITextureDesc{}
		        .setWidth(1)
		        .setHeight(1)
		        .setFormat(RHIFormat::RGBA8_SRGB)
		        .setUsage(RHITextureUsage::Sampled | RHITextureUsage::TransferDest));
		fallback_image->setName("Render.WhiteTexture");
		fallback_view = device->createTextureView(RHITextureViewDesc{}
		        .setTexture(fallback_image.get())
		        .setType(RHITextureViewType::ShaderResource));
		constexpr std::array<uint8, 4> white{255, 255, 255, 255};
		command.writeTexture(fallback_image.get(), RHITextureSlice{}.setExtent(1, 1),
		    white.data(), white.size());
		command.transitionTexture(fallback_image.get(), TransferDest, ShaderResource);
	}

	const auto& requested_meshes = scene.meshes;
	const auto& requested_materials = scene.materials;
	std::unordered_map<uint64, AssetHandle<TextureAsset>> requested_textures;
	for (const auto& [id, material] : requested_materials)
		for (const auto& texture : material.textures)
			if (texture && texture->valid())
				requested_textures.insert_or_assign(texture->getUid(), texture);

	for (const auto& [id, asset] : requested_textures) {
		const auto existing = textures.find(id);
		if (existing != textures.end() && existing->second.revision == asset->getRevision())
			continue;
		RenderTexture resource{};
		resource.revision = asset->getRevision();
		resource.image = device->createTexture(RHITextureDesc{}
		        .setWidth(asset->getWidth())
		        .setHeight(asset->getHeight())
		        .setFormat(RHIFormat::RGBA8_SRGB)
		        .setUsage(RHITextureUsage::Sampled | RHITextureUsage::TransferDest));
		resource.image->setName(asset->getName());
		resource.view = device->createTextureView(RHITextureViewDesc{}
		        .setTexture(resource.image.get())
		        .setType(RHITextureViewType::ShaderResource));
		command.writeTexture(resource.image.get(),
		    RHITextureSlice{}.setExtent(asset->getWidth(), asset->getHeight()),
		    asset->getData().data(), asset->getData().size());
		command.transitionTexture(resource.image.get(), TransferDest, ShaderResource);
		textures.insert_or_assign(id, std::move(resource));
		++texture_epoch;
	}

	for (const auto& [id, asset] : requested_meshes) {
		const auto existing = meshes.find(id);
		if (existing != meshes.end() && existing->second.revision == asset->getRevision())
			continue;
		RenderMesh resource{};
		resource.revision = asset->getRevision();
		const auto& vertices = asset->getVertices();
		const auto& indices = asset->getIndices();
		resource.vertices = device->createBuffer(RHIBufferDesc{}
		        .setSize(vertices.size() * sizeof(MeshVertex))
		        .setStride(sizeof(MeshVertex))
		        .setUsage(RHIBufferUsage::VertexBuffer | RHIBufferUsage::TransferDest));
		resource.indices = device->createBuffer(RHIBufferDesc{}
		        .setSize(indices.size() * sizeof(uint32))
		        .setStride(sizeof(uint32))
		        .setUsage(RHIBufferUsage::IndexBuffer | RHIBufferUsage::TransferDest));
		resource.vertices->setName(std::format("{}.Vertices", asset->getName()));
		resource.indices->setName(std::format("{}.Indices", asset->getName()));
		command.writeBuffer(resource.vertices.get(), 0, vertices.data(),
		    vertices.size() * sizeof(MeshVertex));
		command.writeBuffer(resource.indices.get(), 0, indices.data(),
		    indices.size() * sizeof(uint32));
		command.transitionBuffer(resource.vertices.get(), TransferDest, VertexBuffer);
		command.transitionBuffer(resource.indices.get(), TransferDest, IndexBuffer);
		meshes.insert_or_assign(id, std::move(resource));
	}
	const auto old_texture_count = textures.size();
	std::erase_if(textures, [&](const auto& entry) { return !requested_textures.contains(entry.first); });
	if (textures.size() != old_texture_count)
		++texture_epoch;

	for (const auto& [id, asset] : requested_materials) {
		const auto existing = materials.find(id);
		if (existing != materials.end() && existing->second.revision == asset.revision &&
		    existing->second.texture_epoch == texture_epoch)
			continue;
		RenderMaterial resource{};
		resource.revision = asset.revision;
		resource.texture_epoch = texture_epoch;
		const RenderMaterialConstants constants{
		    .base_color = asset.base_color,
		    .metallic = asset.metallic,
		    .roughness = asset.roughness,
		    .alpha_cutoff = asset.alpha_cutoff,
		    .alpha_mode = static_cast<uint32>(asset.alpha_mode),
		    .emissive_ao = Vec4(asset.emissive, 1.0f),
		};
		resource.constants = uploadBuffer(std::format("{}.Constants", asset.name),
		    RHIBufferDesc{}
		        .setSize(sizeof(constants))
		        .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::TransferDest)
		        .setAccess(RHIAccessMode::Write),
		    std::as_bytes(std::span{&constants, 1}));
		command.transitionBuffer(resource.constants.get(), resource.constants->getState(), ConstantBuffer);
		resource.constant_view = device->createBufferView(RHIBufferViewDesc{}
		        .setBuffer(resource.constants.get())
		        .setType(RHIBufferViewType::Constant));

		auto textureView = [&](MaterialTextureSlot slot) -> RHITextureView* {
			const auto texture = asset.textures[static_cast<size_t>(slot)];
			if (texture) {
				const auto found = textures.find(texture->getUid());
				if (found != textures.end())
					return found->second.view.get();
			}
			return fallback_view.get();
		};
		resource.binding = device->createBindingSet(RHIBindingSetDesc{}
		                                                .addItem(RHIBindingSetItem::constantBuffer(0, resource.constant_view.get()))
		                                                .addItem(RHIBindingSetItem::textureSRV(1, textureView(MaterialTextureSlot::BaseColor)))
		                                                .addItem(RHIBindingSetItem::textureSRV(2, textureView(MaterialTextureSlot::MetallicRoughness)))
		                                                .addItem(RHIBindingSetItem::sampler(3, material_sampler.get())),
		    *material_layout);
		resource.binding->setName(std::format("{}.Material", asset.name));
		materials.insert_or_assign(id, std::move(resource));
	}

	std::erase_if(meshes, [&](const auto& entry) { return !requested_meshes.contains(entry.first); });
	std::erase_if(materials, [&](const auto& entry) { return !requested_materials.contains(entry.first); });
}

const RenderMesh* RenderManager::findMesh(uint64 id) const noexcept
{
	const auto found = meshes.find(id);
	return found == meshes.end() ? nullptr : &found->second;
}

const RenderMaterial* RenderManager::findMaterial(uint64 id) const noexcept
{
	const auto found = materials.find(id);
	return found == materials.end() ? nullptr : &found->second;
}


static auto bindingKey(const RHIBindingLayoutItem& value)
{
	return std::tie(value.type, value.slot, value.size);
}
static auto vertexBindingKey(const RHIVertexBindingDesc& value)
{
	return std::tie(value.binding, value.stride, value.instance);
}
static auto vertexAttributeKey(const RHIVertexAttributeDesc& value)
{
	return std::tie(value.name, value.format, value.location, value.binding, value.offset);
}
static auto samplerKey(const RHISamplerDesc& value)
{
	return std::tie(value.mag_filter, value.min_filter, value.mip_filter, value.mip_bias,
	    value.address_u, value.address_v, value.address_w);
}
static auto blendKey(const RHIColorBlendState::BlendDesc& value)
{
	return std::tie(value.blend_enable, value.src_blend, value.dst_blend,
	    value.color_blend_op, value.src_blend_alpha, value.dst_blend_alpha,
	    value.alpha_blend_op, value.color_write_mask);
}
static auto stencilKey(const RHIDepthStencilState::StencilOp& value)
{
	return std::tie(value.fail_op, value.depth_fail_op, value.pass_op, value.stencil_func);
}
static auto depthKey(const RHIDepthStencilState& value)
{
	return std::tuple(value.depth_test_enable, value.depth_write_enable,
	    value.depth_compare_op, value.depth_bounds_test_enable, value.stencil_test_enable,
	    stencilKey(value.front_face_stencil), stencilKey(value.back_face_stencil));
}
static auto rasterKey(const RHIRasterState& value)
{
	return std::tie(value.polygon_mode, value.cull_mode, value.front_face,
	    value.depth_clamp_enable, value.rasterizer_discard_enable, value.depth_bias,
	    value.depth_bias_clamp, value.depth_bias_slope_factor);
}
static bool samePipeline(const RHIGraphicsPipelineDesc& a, const RHIGraphicsPipelineDesc& b)
{
	return a.primitive_type == b.primitive_type && a.input_layout == b.input_layout &&
	    a.vertex_shader == b.vertex_shader && a.pixel_shader == b.pixel_shader &&
	    a.framebuffer_info == b.framebuffer_info && a.binding_layouts == b.binding_layouts &&
	    a.blend_state.alpha_to_coverage_enable == b.blend_state.alpha_to_coverage_enable &&
	    std::ranges::equal(a.blend_state.blend_descs, b.blend_state.blend_descs, {}, blendKey, blendKey) &&
	    depthKey(a.depth_state) == depthKey(b.depth_state) &&
	    rasterKey(a.raster_state) == rasterKey(b.raster_state);
}

RHIRef<RHIBuffer> RenderManager::uploadBuffer(std::string name, const RHIBufferDesc& desc,
    std::span<const std::byte> bytes)
{
	CHECK(bytes.size() <= desc.size, "Upload data exceeds buffer size");
	auto buffer = device->createBuffer(desc);
	buffer->setName(std::move(name));
	void* mapped = device->mapBuffer(buffer.get(), RHIAccessMode::Write);
	CHECK(mapped, "Failed to map render buffer");
	std::memset(mapped, 0, static_cast<size_t>(desc.size));
	if (!bytes.empty())
		std::memcpy(mapped, bytes.data(), bytes.size());
	device->unmapBuffer(buffer.get());
	return buffer;
}

RHIShader& RenderManager::shader(std::string_view file, RHIShaderType stage, std::string_view entry)
{
	const auto name = std::filesystem::path(file).lexically_normal().generic_string();
	for (const auto& cached : shaders)
		if (cached.file == name && cached.shader->getDesc().type == stage &&
		    cached.shader->getDesc().entry_point == entry)
			return *cached.shader;
	auto path = File::shadersDir() / name;
	if (device->getAPI() == RHIAPI::D3D12)
		path.replace_extension(std::format(".{}.dxil", entry));
	auto result = device->createShader(RHIShaderDesc{}.setType(stage).setShaderName(path.stem().string()).setEntryPoint(std::string(entry)), File::readBinaryFile(path));
	shaders.push_back({name, std::move(result)});
	return *shaders.back().shader;
}

RHIInputLayout& RenderManager::inputLayout(const RHIInputLayoutDesc& desc)
{
	for (const auto& cached : inputs)
		if (std::ranges::equal(cached.desc.binding_descs, desc.binding_descs, {}, vertexBindingKey, vertexBindingKey) &&
		    std::ranges::equal(cached.desc.attribute_descs, desc.attribute_descs, {}, vertexAttributeKey, vertexAttributeKey))
			return *cached.layout;
	inputs.push_back({desc, device->createInputLayout(desc)});
	return *inputs.back().layout;
}

RHIBindingLayout& RenderManager::bindingLayout(const RHIBindingLayoutDesc& desc)
{
	for (const auto& cached : layouts) {
		const auto* cached_desc = cached->getDesc();
		if (cached_desc && cached_desc->visibility == desc.visibility &&
		    std::ranges::equal(cached_desc->bindings, desc.bindings, {}, bindingKey, bindingKey))
			return *cached;
	}
	layouts.push_back(device->createBindingLayout(desc));
	return *layouts.back();
}

RHISampler& RenderManager::sampler(const RHISamplerDesc& desc)
{
	for (const auto& cached : samplers)
		if (samplerKey(cached->getDesc()) == samplerKey(desc))
			return *cached;
	samplers.push_back(device->createSampler(desc));
	return *samplers.back();
}

RHIGraphicsPipeline& RenderManager::graphicsPipeline(const RHIGraphicsPipelineDesc& desc)
{
	for (const auto& cached : graphics_pipelines)
		if (samePipeline(cached->getDesc(), desc))
			return *cached;
	auto result = device->createGraphicsPipeline(desc);
	result->setName(std::format("Render.GraphicsPSO.{}", graphics_pipelines.size()));
	graphics_pipelines.push_back(std::move(result));
	return *graphics_pipelines.back();
}

RHIComputePipeline& RenderManager::computePipeline(const RHIComputePipelineDesc& desc)
{
	for (const auto& cached : compute_pipelines)
		if (cached->getDesc().compute_shader == desc.compute_shader &&
		    cached->getDesc().binding_layouts == desc.binding_layouts)
			return *cached;
	auto result = device->createComputePipeline(desc);
	result->setName(std::format("Render.ComputePSO.{}", compute_pipelines.size()));
	compute_pipelines.push_back(std::move(result));
	return *compute_pipelines.back();
}

}        // namespace Vortex
