#include "RenderScene.hpp"

#include "Render/Graphics/Device.hpp"
#include "Render/RHI/GpuMesh.hpp"
#include "Render/RHI/GpuData.hpp"
#include "Render/RHI/GpuTexture.hpp"
#include "Scene/Components/Mesh.hpp"
#include "Scene/Components/Light.hpp"
#include "Scene/Resources/SubMesh.hpp"
#include "Scene/Resources/Texture.hpp"

constexpr uint32_t MAX_SCENE_SETS = 1;
constexpr uint32_t MAX_MATERIAL_SETS = 10;
constexpr uint32_t MAX_OBJECT_SETS = 100;

RenderScene::RenderScene(Context& context, const World& world) :
    context(&context), world(&world)
{
	createDescriptorLayouts();
	createDescriptorPools();
	createSceneDescriptor();

	rebuild();
}

void RenderScene::createDescriptorLayouts()
{
	auto scene_bindings = std::vector<vk::DescriptorSetLayoutBinding>{GpuSceneData::binding(0)};
	scene_layout = std::make_unique<DescriptorSetLayout>(*context, scene_bindings);

	auto material_bindings = GpuMaterialData::bindings(0);
	material_layout = std::make_unique<DescriptorSetLayout>(*context, material_bindings);

	auto object_bindings = std::vector<vk::DescriptorSetLayoutBinding>{GpuObjectData::binding(0)};
	object_layout = std::make_unique<DescriptorSetLayout>(*context, object_bindings);
}

void RenderScene::createDescriptorPools()
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

void RenderScene::createSceneDescriptor()
{
	scene_data.view = glm::mat4(1.0f);
	scene_data.projection = glm::mat4(1.0f);
	scene_data.ambient_color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	scene_uniform = Buffer::createDynamic(
	    *context,
	    vk::BufferUsageFlagBits::eUniformBuffer,
	    &scene_data,
	    sizeof(GpuSceneData));

	scene_descriptor = scene_pool->allocate(*scene_layout);
	scene_descriptor.update(context->getDevice(), 0, vk::DescriptorType::eUniformBuffer, scene_uniform.get());
}

void RenderScene::loadTextures()
{
	if (!world || !world->getActiveScene())
		return;

	auto textures = world->getActiveScene()->getResources<Texture>();

	gpu_textures.clear();
	texture_to_gpu_texture.clear();

	default_sampler = std::make_shared<Sampler>(*context);
	for (auto texture : textures) {
		if (!texture || !texture->valid())
			continue;

		auto gpu_texture = std::make_unique<GpuTexture>(*context, texture, default_sampler);
		texture_to_gpu_texture[texture] = gpu_texture.get();
		gpu_textures.push_back(std::move(gpu_texture));
	}
}

void RenderScene::loadMaterials()
{
	if (!world || !world->getActiveScene())
		return;

	auto materials = world->getActiveScene()->getResources<Material>();

	gpu_materials.clear();
	gpu_materials.reserve(materials.size());

	for (auto material : materials) {
		if (!material)
			continue;

		Image* base_color_texture = nullptr;
		Image* metallic_roughness_texture = nullptr;

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

void RenderScene::loadMeshes()
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

void RenderScene::organizeMeshesByMaterial()
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

bool RenderScene::needsRebuild() const
{
	if (!world || !world->getActiveScene())
		return false;

	auto submeshes = world->getActiveScene()->getResources<SubMesh>();
	auto materials = world->getActiveScene()->getResources<Material>();

	return submeshes.size() != last_submesh_count || materials.size() != last_material_count;
}

void RenderScene::clear()
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

void RenderScene::updateCamera()
{
	if (!world)
		return;

	auto* camera = world->getActiveCamera();
	if (camera) {
		scene_data.view = camera->getView();
		scene_data.projection = camera->getProjection();

		glm::mat4 inv_view = glm::inverse(scene_data.view);
		scene_data.camera_position = glm::vec4(inv_view[3][0], inv_view[3][1], inv_view[3][2], 1.0f);
	}

	updateLights();

	scene_uniform->upload(&scene_data, sizeof(GpuSceneData));
}

void RenderScene::updateLights()
{
	if (!world || !world->getActiveScene())
		return;

	auto scene = world->getActiveScene();
	auto lights = scene->getComponents<Light>();

	scene_data.light_count = std::min(static_cast<uint32_t>(lights.size()), MAX_LIGHTS);

	for (uint32_t i = 0; i < scene_data.light_count; ++i) {
		auto* light = lights[i];
		auto& gpu_light = scene_data.lights[i];
		Node* light_node = nullptr;

		std::function<Node*(Node*)> findLightNode = [&](Node* node) -> Node* {
			if (!node)
				return nullptr;

			if (node->hasComponent<Light>() && &node->getComponent<Light>() == light)
				return node;

			for (auto* child : node->getChildren())
				if (auto* found = findLightNode(child))
					return found;

			return nullptr;
		};

		for (auto* root = scene->getRoot(); auto* child : root->getChildren()) {
			light_node = findLightNode(child);
			if (light_node)
				break;
		}

		glm::vec3 position(0.0f);
		glm::vec3 direction(0.0f, -1.0f, 0.0f);

		if (light_node) {
			auto world_matrix = getWorldMatrix(light_node);
			position = glm::vec3(world_matrix[3]);
			direction = glm::normalize(glm::vec3(world_matrix * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
		}

		if (auto* dir_light = dynamic_cast<DirectionalLight*>(light)) {
			gpu_light.position = glm::vec4(direction, 0.0f);
			gpu_light.direction = glm::vec4(direction, 0.0f);
			gpu_light.color = glm::vec4(dir_light->getColor(), dir_light->getIntensity());
			gpu_light.params = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

		} else if (auto* point_light = dynamic_cast<PointLight*>(light)) {
			gpu_light.position = glm::vec4(position, 1.0f);
			gpu_light.direction = glm::vec4(0.0f);
			gpu_light.color = glm::vec4(point_light->getColor(), point_light->getIntensity());
			gpu_light.params = glm::vec4(point_light->getRange(), 0.0f, 0.0f, 1.0f);

		} else if (auto* spot_light = dynamic_cast<SpotLight*>(light)) {
			gpu_light.position = glm::vec4(position, 1.0f);
			gpu_light.direction = glm::vec4(direction, 0.0f);
			gpu_light.color = glm::vec4(spot_light->getColor(), spot_light->getIntensity());
			gpu_light.params = glm::vec4(spot_light->getRange(), spot_light->getInnerConeAngle(), spot_light->getOuterConeAngle(), 2.0f);
		}
	}
}

void RenderScene::updateMesh()
{
	if (!world || !world->getActiveScene())
		return;

	auto scene = world->getActiveScene();

	std::function<void(Node*)> traverse = [&](Node* node) {
		if (!node)
			return;

		if (node->hasComponent<Mesh>()) {
			auto& mesh = node->getComponent<Mesh>();
			auto  world_matrix = getWorldMatrix(node);

			for (auto submesh : mesh.getSubmeshes()) {
				auto it = submesh_to_gpu_mesh.find(submesh);
				if (it != submesh_to_gpu_mesh.end()) {
					it->second->setModelMatrix(world_matrix);
					it->second->updateUniforms();
				}
			}
		}

		for (auto* child : node->getChildren())
			traverse(child);
	};

	auto* root = scene->getRoot();
	for (auto* child : root->getChildren())
		traverse(child);
}

glm::mat4 RenderScene::getWorldMatrix(const Node* node) const
{
	if (!node)
		return glm::mat4(1.0f);

	auto& transform = const_cast<Node*>(node)->getTransform();
	return transform.getWorldMatrix();
}

void RenderScene::update(float dt)
{
	if (!world || !world->getActiveScene())
		return;

	if (needsRebuild())
		rebuild();

	updateCamera();
	updateMesh();
}

void RenderScene::rebuild()
{
	context->getDevice().logical().waitIdle();

	clear();

	if (!world || !world->getActiveScene())
		return;

	auto materials = world->getActiveScene()->getResources<Material>();
	auto submeshes = world->getActiveScene()->getResources<SubMesh>();

	if (materials.size() > material_pool->setsCount()) {
		uint32_t                            material_count = std::max(1u, static_cast<uint32_t>(materials.size()));
		std::vector<vk::DescriptorPoolSize> material_pool_sizes = {
		    {vk::DescriptorType::eUniformBuffer, material_count},
		    {vk::DescriptorType::eCombinedImageSampler, material_count * 5},
		};
		material_pool = std::make_unique<DescriptorPool>(*context, material_count, material_pool_sizes);
	}

	if (submeshes.size() > object_pool->setsCount()) {
		uint32_t                            mesh_count = std::max(1u, static_cast<uint32_t>(submeshes.size()));
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

void RenderScene::draw(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout)
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

std::vector<vk::DescriptorSetLayout> RenderScene::getDescriptorSetLayouts() const
{
	return {
	    scene_layout->get(),
	    material_layout->get(),
	    object_layout->get(),
	};
}

DescriptorSetLayout* RenderScene::getSceneLayout()
{
	return scene_layout.get();
}

DescriptorSetLayout* RenderScene::getMaterialLayout()
{
	return material_layout.get();
}

DescriptorSetLayout* RenderScene::getObjectLayout()
{
	return object_layout.get();
}

DescriptorSet RenderScene::getSceneDescriptor()
{
	return scene_descriptor;
}

const World* RenderScene::getWorld() const
{
	return world;
}
