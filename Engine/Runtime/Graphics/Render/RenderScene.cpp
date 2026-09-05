module Runtime.Graphics;

namespace Vortex {

namespace {

void hashCombine(size_t& seed, size_t value) noexcept
{
	seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

template <IsAsset T>
void hashAssetHandle(size_t& seed, const AssetHandle<T>& asset) noexcept
{
	if (!asset) {
		hashCombine(seed, 0);
		return;
	}
	hashCombine(seed, std::hash<uint64>{}(asset->getUid()));
	hashCombine(seed, std::hash<uint64>{}(asset->getRevision()));
}

}        // namespace

RenderScene::RenderScene(RHIContext& context, const World& world) :
    context(&context), world(&world)
{
	createSceneLayouts();
	createSceneBindingSet();
	rebuild();
}

void RenderScene::createSceneLayouts()
{
	scene_layout = context->getDevice().createBindingLayout(RenderSceneData::layout(0));
	material_layout = context->getDevice().createBindingLayout(RenderMaterialData::layout(0));
	object_layout = context->getDevice().createBindingLayout(RenderObjectData::layout(0));
}

void RenderScene::createSceneBindingSet()
{
	RHIBufferDesc constant_buffer_desc{};
	constant_buffer_desc.setSize(sizeof(RenderSceneData))
	    .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest)
	    .setAccess(RHIAccessMode::Write);
	scene_constant_buffer = context->getDevice().createBuffer(constant_buffer_desc);
	scene_constant_buffer_view = context->getDevice().createBufferView(
	    RHIBufferViewDesc{}.setBuffer(scene_constant_buffer.get()).setType(RHIBufferViewType::Constant));

	RHIBindingSetDesc set_desc{};
	set_desc.addItem(RHIBindingSetItem().setBufferView(0, scene_constant_buffer_view.get()));
	scene_binding_set = context->getDevice().createBindingSet(set_desc, *scene_layout);
}

void RenderScene::updateCamera()
{
	if (world)
		if (auto* camera = world->getActiveCamera())
			scene_data.camera = RenderCameraData::convert(*camera);
}

void RenderScene::updateLights()
{
	if (!world || !world->getActiveScene())
		return;

	auto lights = world->getActiveScene()->getComponents<LightComponent>();
	scene_data.light_count = std::min(static_cast<uint32>(lights.size()), MAX_LIGHTS_COUNT);
	for (uint32 index = 0; index < scene_data.light_count; ++index)
		scene_data.lights[index] = RenderLightData::convert(*lights[index]);
}

void RenderScene::updateMeshes()
{
	for (const auto& [component, render_mesh] : render_mesh_map) {
		if (!component || !render_mesh)
			continue;
		render_mesh->setModelMatrix(component->getWorldMatrix());
		render_mesh->updateUniforms();
	}
}

void RenderScene::collectAssets(std::vector<MeshComponent*>& meshes,
    std::vector<AssetHandle<MaterialAsset>>& materials,
    std::vector<AssetHandle<TextureAsset>>& textures) const
{
	if (!world || !world->getActiveScene())
		return;

	std::unordered_set<uint64> material_ids;
	std::unordered_set<uint64> texture_ids;

	for (auto* mesh_component : world->getActiveScene()->getComponents<MeshComponent>()) {
		if (!mesh_component || !mesh_component->isVisible() || !mesh_component->getMesh() || !mesh_component->getMesh()->valid())
			continue;

		meshes.push_back(mesh_component);
		for (const auto& section : mesh_component->getMesh()->getSections()) {
			auto material = mesh_component->getMaterial(section.material_slot);
			if (material && material_ids.insert(material->getUid()).second)
				materials.push_back(std::move(material));
		}
	}

	for (const auto& material : materials)
		for (const auto& [name, texture] : material->getTextures())
			if (texture && texture_ids.insert(texture->getUid()).second)
				textures.push_back(texture);
}

void RenderScene::loadTextures(const std::vector<AssetHandle<TextureAsset>>& textures)
{
	material_sampler = context->getDevice().createSampler(RHISamplerDesc{});
	render_textures.reserve(textures.size());
	for (const auto& texture : textures) {
		if (!texture || !texture->valid())
			continue;

		auto render_texture = std::make_unique<RenderTexture>(*context, texture, material_sampler);
		render_texture_map.insert_or_assign(texture->getUid(), render_texture.get());
		render_textures.push_back(std::move(render_texture));
	}
}

void RenderScene::loadMaterials(const std::vector<AssetHandle<MaterialAsset>>& materials)
{
	render_materials.reserve(materials.size());
	for (const auto& material : materials) {
		if (!material)
			continue;

		RHITextureView* base_color_texture = nullptr;
		RHITextureView* metallic_roughness_texture = nullptr;
		RHISampler* sampler = material_sampler.get();

		if (auto texture = material->getTexture("baseColor"); texture) {
			auto texture_it = render_texture_map.find(texture->getUid());
			if (texture_it != render_texture_map.end()) {
				base_color_texture = texture_it->second->getTextureView();
				sampler = texture_it->second->getSampler();
			}
		}

		if (auto texture = material->getTexture("metallicRoughness"); texture) {
			auto texture_it = render_texture_map.find(texture->getUid());
			if (texture_it != render_texture_map.end())
				metallic_roughness_texture = texture_it->second->getTextureView();
		}

		auto render_material = std::make_unique<RenderMaterial>(
		    *context,
		    material,
		    *material_layout,
		    base_color_texture,
		    metallic_roughness_texture,
		    sampler);
		render_material_map.insert_or_assign(material->getUid(), render_material.get());
		render_materials.push_back(std::move(render_material));
	}
}

void RenderScene::loadMeshes(const std::vector<MeshComponent*>& meshes)
{
	render_meshes.reserve(meshes.size());
	for (auto* component : meshes) {
		auto render_mesh = std::make_unique<RenderMesh>(*context, component->getMesh(), *object_layout);
		render_mesh->setModelMatrix(component->getWorldMatrix());
		render_mesh->updateUniforms();
		render_mesh_map.insert_or_assign(component, render_mesh.get());
		render_meshes.push_back(std::move(render_mesh));
	}
}

void RenderScene::sortMeshes(const std::vector<MeshComponent*>& meshes)
{
	for (auto* component : meshes) {
		auto render_mesh_it = render_mesh_map.find(component);
		if (render_mesh_it == render_mesh_map.end())
			continue;

		const auto& sections = component->getMesh()->getSections();
		for (uint32 section_index = 0; section_index < sections.size(); ++section_index) {
			auto material = component->getMaterial(sections[section_index].material_slot);
			if (material && render_material_map.contains(material->getUid()))
				meshes_by_material[material->getUid()].push_back({render_mesh_it->second, section_index});
		}
	}
}

size_t RenderScene::calculateAssetState() const
{
	if (!world || !world->getActiveScene())
		return 0;

	size_t state = 0;
	for (auto* component : world->getActiveScene()->getComponents<MeshComponent>()) {
		hashCombine(state, std::hash<uint64>{}(component->getUid()));
		hashCombine(state, std::hash<bool>{}(component->isVisible()));

		const auto& mesh = component->getMesh();
		hashAssetHandle(state, mesh);
		if (!mesh)
			continue;

		for (const auto& section : mesh->getSections()) {
			auto material = component->getMaterial(section.material_slot);
			hashAssetHandle(state, material);
			if (!material)
				continue;
			for (const auto& [name, texture] : material->getTextures()) {
				hashCombine(state, std::hash<std::string>{}(name));
				hashAssetHandle(state, texture);
			}
		}
	}
	return state;
}

bool RenderScene::needsRebuild() const
{
	return calculateAssetState() != asset_state;
}

void RenderScene::clear()
{
	meshes_by_material.clear();
	render_mesh_map.clear();
	render_meshes.clear();
	render_material_map.clear();
	render_materials.clear();
	render_texture_map.clear();
	render_textures.clear();
}

void RenderScene::update(float)
{
	if (!world || !world->getActiveScene())
		return;

	if (needsRebuild())
		rebuild();

	updateCamera();
	updateLights();
	updateMeshes();

	void* mapped = context->getDevice().mapBuffer(scene_constant_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, &scene_data, sizeof(RenderSceneData));
	context->getDevice().unmapBuffer(scene_constant_buffer.get());
}

void RenderScene::rebuild()
{
	clear();

	std::vector<MeshComponent*> meshes;
	std::vector<AssetHandle<MaterialAsset>> materials;
	std::vector<AssetHandle<TextureAsset>> textures;
	collectAssets(meshes, materials, textures);
	loadTextures(textures);
	loadMaterials(materials);
	loadMeshes(meshes);
	sortMeshes(meshes);
	asset_state = calculateAssetState();
}

void RenderScene::draw(RHICommandList& command, const RHIGraphicsState& base_state)
{
	if (!scene_binding_set)
		return;

	auto scene_state = base_state;
	scene_state.addBindingSet(scene_binding_set.get());

	for (const auto& [material_id, draws] : meshes_by_material) {
		auto material_it = render_material_map.find(material_id);
		if (material_it == render_material_map.end())
			continue;

		auto material_state = scene_state;
		material_it->second->updateGraphicsState(material_state);

		for (const auto& draw : draws) {
			if (!draw.mesh || !draw.mesh->getSourceMesh())
				continue;
			const auto& sections = draw.mesh->getSourceMesh()->getSections();
			if (draw.section_index >= sections.size())
				continue;

			auto mesh_state = material_state;
			draw.mesh->updateGraphicsState(mesh_state);
			command.setGraphicsState(mesh_state);
			draw.mesh->draw(command, sections[draw.section_index]);
		}
	}
}

}        // namespace Vortex
