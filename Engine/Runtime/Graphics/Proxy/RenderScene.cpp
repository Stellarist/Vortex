module Runtime.Graphics;

namespace Vortex {

constexpr uint32 MAX_SCENE_SETS = 1;
constexpr uint32 MAX_MATERIAL_SETS = 10;
constexpr uint32 MAX_OBJECT_SETS = 100;

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
	if (!world)
		return;

	auto* camera = world->getActiveCamera();
	if (!camera)
		return;

	scene_data.camera = RenderCameraData::convert(*camera);
}

void RenderScene::updateLights()
{
	if (!world)
		return;

	auto* scene = world->getActiveScene();
	if (!scene)
		return;

	auto lights = scene->getComponents<Light>();
	if (lights.empty())
		return;

	scene_data.light_count = std::min(static_cast<uint32>(lights.size()), MAX_LIGHTS_COUNT);
	for (uint32 i = 0; i < scene_data.light_count; i++)
		scene_data.lights[i] = RenderLightData::convert(*lights[i]);
}

void RenderScene::updateMesh()
{
	if (!world || !world->getActiveScene())
		return;

	auto scene = world->getActiveScene();
	auto meshes = scene->getComponents<Mesh>();

	for (auto& mesh : meshes) {
		auto transform = mesh->getNode()->getTransform();
		for (auto submesh : mesh->getSubmeshes()) {
			auto it = render_mesh_map.find(submesh);
			if (it != render_mesh_map.end()) {
				it->second->setModelMatrix(transform.getWorldMatrix());
				it->second->updateUniforms();
			}
		}
	}
}

void RenderScene::loadTextures()
{
	if (!world || !world->getActiveScene())
		return;

	auto textures = world->getActiveScene()->getResources<Texture>();
	render_textures.clear();
	render_textures.reserve(textures.size());
	render_texture_map.clear();

	material_sampler = context->getDevice().createSampler(RHISamplerDesc{});
	for (auto texture : textures) {
		if (!texture || !texture->valid())
			continue;

		auto render_texture = std::make_unique<RenderTexture>(*context, texture, material_sampler);
		render_texture_map[texture] = render_texture.get();
		render_textures.push_back(std::move(render_texture));
	}

	last_texture_count = textures.size();
}

void RenderScene::loadMaterials()
{
	if (!world || !world->getActiveScene())
		return;

	auto materials = world->getActiveScene()->getResources<Material>();
	render_materials.clear();
	render_materials.reserve(materials.size());

	for (auto material : materials) {
		if (!material)
			continue;

		RHITextureView* base_color_texture = nullptr;
		RHITextureView* metallic_roughness_texture = nullptr;
		RHISampler*     mat_sampler = material_sampler.get();

		if (auto base_color_tex = material->getTexture("baseColor"); base_color_tex)
			if (auto base_color_it = render_texture_map.find(base_color_tex); base_color_it != render_texture_map.end()) {
				base_color_texture = base_color_it->second->getTextureView();
				mat_sampler = base_color_it->second->getSampler();
			}

		if (auto metallic_roughness_tex = material->getTexture("metallicRoughness"); metallic_roughness_tex)
			if (auto metallic_roughness_it = render_texture_map.find(metallic_roughness_tex); metallic_roughness_it != render_texture_map.end())
				metallic_roughness_texture = metallic_roughness_it->second->getTextureView();

		auto render_mat = std::make_unique<RenderMaterial>(
		    *context,
		    material,
		    *material_layout,
		    base_color_texture,
		    metallic_roughness_texture,
		    mat_sampler);

		render_materials.push_back(std::move(render_mat));
	}

	last_material_count = materials.size();
}

void RenderScene::loadMeshes()
{
	if (!world || !world->getActiveScene())
		return;

	auto submeshes = world->getActiveScene()->getResources<SubMesh>();

	render_meshes.clear();
	render_mesh_map.clear();
	render_meshes.reserve(submeshes.size());

	for (auto submesh : submeshes) {
		if (submesh && submesh->isVisible()) {
			auto render_mesh = std::make_unique<RenderMesh>(
			    *context,
			    submesh,
			    *object_layout);

			render_mesh_map[submesh] = render_mesh.get();
			render_meshes.push_back(std::move(render_mesh));
		}
	}

	last_submesh_count = submeshes.size();
}

void RenderScene::sortMeshes()
{
	meshes_by_material.clear();

	if (!world || !world->getActiveScene())
		return;

	for (auto& render_mesh : render_meshes) {
		auto submesh = render_mesh->getSrcSubMesh();
		auto material = submesh->getMaterial();

		if (!material && !render_materials.empty())
			material = render_materials[0]->getSrcMaterial();

		if (material)
			meshes_by_material[material].push_back(render_mesh.get());
	}
}

bool RenderScene::needsRebuild() const
{
	if (!world || !world->getActiveScene())
		return false;

	const auto& textures = world->getActiveScene()->getResources<Texture>();
	const auto& submeshes = world->getActiveScene()->getResources<SubMesh>();
	const auto& materials = world->getActiveScene()->getResources<Material>();

	return submeshes.size() != last_submesh_count || materials.size() != last_material_count || textures.size() != last_texture_count;
}

void RenderScene::clear()
{
	meshes_by_material.clear();
	render_mesh_map.clear();
	render_meshes.clear();
	render_materials.clear();
	render_textures.clear();
	render_texture_map.clear();
}

void RenderScene::update(float dt)
{
	if (!world || !world->getActiveScene())
		return;

	if (needsRebuild())
		rebuild();

	updateCamera();
	updateLights();
	updateMesh();

	void* mapped = context->getDevice().mapBuffer(scene_constant_buffer.get(), RHIAccessMode::Write);
	std::memcpy(mapped, &scene_data, sizeof(RenderSceneData));
	context->getDevice().unmapBuffer(scene_constant_buffer.get());
}

void RenderScene::rebuild()
{
	clear();

	if (!world || !world->getActiveScene())
		return;

	loadTextures();
	loadMaterials();
	loadMeshes();
	sortMeshes();
}

void RenderScene::draw(RHICommandList& command, const RHIGraphicsState& base_state)
{
	if (!scene_binding_set)
		return;

	auto scene_state = base_state;
	scene_state.addBindingSet(scene_binding_set.get());

	for (auto& [material, meshes] : meshes_by_material) {
		RenderMaterial* render_material = nullptr;
		for (auto& gm : render_materials) {
			if (gm->getSrcMaterial() == material) {
				render_material = gm.get();
				break;
			}
		}

		if (!render_material)
			continue;

		auto material_state = scene_state;
		render_material->updateGraphicsState(material_state);

		for (auto* mesh : meshes) {
			auto mesh_state = material_state;
			mesh->updateGraphicsState(mesh_state);
			command.setGraphicsState(mesh_state);
			mesh->draw(command);
		}
	}
}

}        // namespace Vortex
