#include "GpuScene.hpp"

#include "Runtime/Render/Resources/GpuMesh.hpp"
#include "Runtime/Render/Resources/GpuData.hpp"
#include "Runtime/Render/Resources/GpuTexture.hpp"
#include "Runtime/World/Components/Mesh.hpp"
#include "Runtime/World/Components/Light.hpp"
#include "Runtime/World/Resources/SubMesh.hpp"
#include "Runtime/World/Resources/Texture.hpp"

constexpr uint32_t MAX_SCENE_SETS = 1;
constexpr uint32_t MAX_MATERIAL_SETS = 10;
constexpr uint32_t MAX_OBJECT_SETS = 100;

GpuScene::GpuScene(VulkanContext& context, const World& world) :
    context(&context), world(&world)
{
	createDescriptorLayouts();
	createDescriptorPools();
	createSceneDescriptor();

	rebuild();
}

void GpuScene::createDescriptorLayouts()
{
	auto scene_bindings = std::vector<vk::DescriptorSetLayoutBinding>{GpuSceneData::binding(0)};
	scene_layout = std::make_unique<VulkanDescriptorSetLayout>(*context, scene_bindings);

	auto material_bindings = GpuMaterialData::bindings(0);
	material_layout = std::make_unique<VulkanDescriptorSetLayout>(*context, material_bindings);

	auto object_bindings = std::vector<vk::DescriptorSetLayoutBinding>{GpuObjectData::binding(0)};
	object_layout = std::make_unique<VulkanDescriptorSetLayout>(*context, object_bindings);
}

void GpuScene::createDescriptorPools()
{
	std::vector<vk::DescriptorPoolSize> scene_pool_sizes = {{vk::DescriptorType::eUniformBuffer, MAX_SCENE_SETS}};
	scene_pool = std::make_unique<DescriptorPool>(*context, MAX_SCENE_SETS, scene_pool_sizes);

	std::vector<vk::DescriptorPoolSize> material_pool_sizes = {
	    {vk::DescriptorType::eUniformBuffer, MAX_MATERIAL_SETS},
	    {vk::DescriptorType::eCombinedImageSampler, MAX_MATERIAL_SETS * 5}};
	material_pool = std::make_unique<DescriptorPool>(*context, MAX_MATERIAL_SETS, material_pool_sizes);

	std::vector<vk::DescriptorPoolSize> object_pool_sizes = {{vk::DescriptorType::eUniformBuffer, MAX_OBJECT_SETS}};
	object_pool = std::make_unique<DescriptorPool>(*context, MAX_OBJECT_SETS, object_pool_sizes);
}

void GpuScene::createSceneDescriptor()
{
	scene_uniform = VulkanBuffer::createDynamic(*context, vk::BufferUsageFlagBits::eUniformBuffer, &scene_data, sizeof(GpuSceneData));

	scene_descriptor = scene_pool->allocate(*scene_layout);
	scene_descriptor.update(context->getDevice(), 0, vk::DescriptorType::eUniformBuffer, scene_uniform.get());
}

void GpuScene::updateCamera()
{
	if (!world)
		return;

	auto* camera = world->getActiveCamera();
	if (!camera)
		return;

	if (!camera->getNode()->getTransform().dirty())
		return;

	scene_data.camera = GpuCameraData::convert(*camera);
}

void GpuScene::updateLights()
{
	if (!world)
		return;

	auto* scene = world->getActiveScene();
	if (!scene)
		return;

	auto lights = scene->getComponents<Light>();
	if (lights.empty())
		return;

	scene_data.light_count = std::min(static_cast<uint32_t>(lights.size()), MAX_LIGHTS);
	for (uint32_t i = 0; i < scene_data.light_count; i++)
		scene_data.lights[i] = GpuLightData::convert(*lights[i]);
}

void GpuScene::updateMesh()
{
	if (!world || !world->getActiveScene())
		return;

	auto scene = world->getActiveScene();

	auto meshes = scene->getComponents<Mesh>();
	for (auto& mesh : meshes) {
		auto transform = mesh->getNode()->getTransform();
		if (!transform.dirty())
			continue;

		for (auto submesh : mesh->getSubmeshes()) {
			auto it = submesh_to_gpu_mesh.find(submesh);
			if (it != submesh_to_gpu_mesh.end()) {
				it->second->setModelMatrix(transform.getWorldMatrix());
				it->second->updateUniforms();
			}
		}
	}
}

void GpuScene::loadTextures()
{
	if (!world || !world->getActiveScene())
		return;

	auto textures = world->getActiveScene()->getResources<Texture>();
	gpu_textures.clear();
	gpu_textures.reserve(textures.size());
	texture_to_gpu_texture.clear();

	default_sampler = std::make_shared<VulkanSampler>(*context);
	for (auto texture : textures) {
		if (!texture || !texture->valid())
			continue;

		auto gpu_texture = std::make_unique<GpuTexture>(*context, texture, default_sampler);
		texture_to_gpu_texture[texture] = gpu_texture.get();
		gpu_textures.push_back(std::move(gpu_texture));
	}

	last_texture_count = textures.size();
}

void GpuScene::loadMaterials()
{
	if (!world || !world->getActiveScene())
		return;

	auto materials = world->getActiveScene()->getResources<Material>();
	gpu_materials.clear();
	gpu_materials.reserve(materials.size());

	for (auto material : materials) {
		if (!material)
			continue;

		VulkanImage* base_color_texture = nullptr;
		VulkanImage* metallic_roughness_texture = nullptr;

		if (auto base_color_tex = material->getTexture("baseColor"); base_color_tex)
			if (auto base_color_it = texture_to_gpu_texture.find(base_color_tex); base_color_it != texture_to_gpu_texture.end())
				base_color_texture = base_color_it->second->getImage();

		if (auto metallic_roughness_tex = material->getTexture("metallicRoughness"); metallic_roughness_tex)
			if (auto metallic_roughness_it = texture_to_gpu_texture.find(metallic_roughness_tex); metallic_roughness_it != texture_to_gpu_texture.end())
				metallic_roughness_texture = metallic_roughness_it->second->getImage();

		auto gpu_material = std::make_unique<GpuMaterial>(
		    *context,
		    material,
		    *material_layout,
		    *material_pool,
		    base_color_texture,
		    metallic_roughness_texture);

		gpu_materials.push_back(std::move(gpu_material));
	}

	last_material_count = materials.size();
}

void GpuScene::loadMeshes()
{
	if (!world || !world->getActiveScene())
		return;

	auto submeshes = world->getActiveScene()->getResources<SubMesh>();

	gpu_meshes.clear();
	submesh_to_gpu_mesh.clear();
	gpu_meshes.reserve(submeshes.size());

	for (auto submesh : submeshes) {
		if (submesh && submesh->isVisible()) {
			auto gpu_mesh = std::make_unique<GpuMesh>(
			    *context,
			    *submesh,
			    *object_layout,
			    *object_pool);

			submesh_to_gpu_mesh[submesh] = gpu_mesh.get();
			gpu_meshes.push_back(std::move(gpu_mesh));
		}
	}

	last_submesh_count = submeshes.size();
}

void GpuScene::organizeMeshesByMaterial()
{
	meshes_by_material.clear();

	if (!world || !world->getActiveScene())
		return;

	for (auto& gpu_mesh : gpu_meshes) {
		auto submesh = gpu_mesh->getSubMesh();
		auto material = submesh->getMaterial();

		if (!material && !gpu_materials.empty())
			material = gpu_materials[0]->getSourceMaterial();

		if (material)
			meshes_by_material[material].push_back(gpu_mesh.get());
	}
}

bool GpuScene::needsRebuild() const
{
	if (!world || !world->getActiveScene())
		return false;

	const auto& textures = world->getActiveScene()->getResources<Texture>();
	const auto& submeshes = world->getActiveScene()->getResources<SubMesh>();
	const auto& materials = world->getActiveScene()->getResources<Material>();

	return submeshes.size() != last_submesh_count || materials.size() != last_material_count || textures.size() != last_texture_count;
}

void GpuScene::clear()
{
	meshes_by_material.clear();
	submesh_to_gpu_mesh.clear();
	gpu_meshes.clear();
	gpu_materials.clear();
	gpu_textures.clear();
	texture_to_gpu_texture.clear();

	if (material_pool)
		material_pool->reset();
	if (object_pool)
		object_pool->reset();
}

void GpuScene::update(float dt)
{
	if (!world || !world->getActiveScene())
		return;

	if (needsRebuild())
		rebuild();

	updateCamera();
	updateLights();
	updateMesh();

	scene_uniform->upload(&scene_data, sizeof(GpuSceneData));
}

void GpuScene::rebuild()
{
	clear();

	if (!world || !world->getActiveScene())
		return;

	auto     materials = world->getActiveScene()->getResources<Material>();
	uint32_t material_count = std::max(1u, static_cast<uint32_t>(materials.size()));
	if (material_pool->setsCount() < materials.size()) {
		std::vector<vk::DescriptorPoolSize> material_pool_sizes = {
		    {vk::DescriptorType::eUniformBuffer, material_count},
		    {vk::DescriptorType::eCombinedImageSampler, material_count * 5},
		};
		material_pool = std::make_unique<DescriptorPool>(*context, material_count, material_pool_sizes);
	}

	auto     submeshes = world->getActiveScene()->getResources<SubMesh>();
	uint32_t mesh_count = std::max(1u, static_cast<uint32_t>(submeshes.size()));
	if (object_pool->setsCount() < submeshes.size()) {
		std::vector<vk::DescriptorPoolSize> object_pool_sizes = {
		    {vk::DescriptorType::eUniformBuffer, mesh_count * 2},
		};
		object_pool = std::make_unique<DescriptorPool>(*context, mesh_count * 2, object_pool_sizes);
	}

	loadTextures();
	loadMaterials();
	loadMeshes();
	organizeMeshesByMaterial();
}

void GpuScene::draw(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout)
{
	command_buffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    pipeline_layout,
	    0,
	    scene_descriptor.get(),
	    {});

	for (auto& [material, meshes] : meshes_by_material) {
		GpuMaterial* gpu_material = nullptr;
		for (auto& gm : gpu_materials) {
			if (gm->getSourceMaterial() == material) {
				gpu_material = gm.get();
				break;
			}
		}

		if (!gpu_material)
			continue;
		gpu_material->bind(command_buffer, pipeline_layout);

		for (auto* mesh : meshes) {
			mesh->bind(command_buffer, pipeline_layout);
			mesh->draw(command_buffer);
		}
	}
}

std::vector<vk::DescriptorSetLayout> GpuScene::getDescriptorSetLayouts() const
{
	return {
	    scene_layout->get(),
	    material_layout->get(),
	    object_layout->get(),
	};
}

VulkanDescriptorSetLayout* GpuScene::getSceneLayout()
{
	return scene_layout.get();
}

VulkanDescriptorSetLayout* GpuScene::getMaterialLayout()
{
	return material_layout.get();
}

VulkanDescriptorSetLayout* GpuScene::getObjectLayout()
{
	return object_layout.get();
}

VulkanDescriptorSet GpuScene::getSceneDescriptor()
{
	return scene_descriptor;
}

const World* GpuScene::getWorld() const
{
	return world;
}
