#include "RenderScene.hpp"

#include "Render/RHI/GpuMesh.hpp"
#include "Render/RHI/GpuData.hpp"
#include "Scene/Components/Mesh.hpp"
#include "Scene/Resources/SubMesh.hpp"

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
	std::vector<vk::DescriptorPoolSize> scene_pool_sizes = {{vk::DescriptorType::eUniformBuffer, 1}};
	scene_pool = std::make_unique<DescriptorPool>(*context, 1, scene_pool_sizes);

	std::vector<vk::DescriptorPoolSize> material_pool_sizes = {
	    {vk::DescriptorType::eUniformBuffer, 10},
	    {vk::DescriptorType::eCombinedImageSampler, 10},
	};
	material_pool = std::make_unique<DescriptorPool>(*context, 10, material_pool_sizes);

	std::vector<vk::DescriptorPoolSize> object_pool_sizes = {{vk::DescriptorType::eUniformBuffer, 100}};
	object_pool = std::make_unique<DescriptorPool>(*context, 100, object_pool_sizes);
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

	auto set_index = scene_pool->allocate(*scene_layout);
	scene_descriptor = std::make_unique<DescriptorSet>(*context, scene_pool->getSet(set_index).get());
	scene_descriptor->update(0, vk::DescriptorType::eUniformBuffer, scene_uniform.get());
}

void RenderScene::loadMaterials()
{
	if (!world || !world->getActiveScene())
		return;

	auto materials = world->getActiveScene()->getResources<Material>();

	gpu_materials.clear();
	gpu_materials.reserve(materials.size());

	for (auto material : materials) {
		if (material) {
			gpu_materials.push_back(std::make_unique<GpuMaterial>(
			    *context,
			    material,
			    *material_layout,
			    *material_pool,
			    default_texture.get()));
		}
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

	if (material_pool)
		material_pool->reset();
	if (object_pool)
		object_pool->reset();
}

void RenderScene::updateCameraTransform()
{
	if (!world)
		return;

	auto* camera = world->getActiveCamera();
	if (camera) {
		scene_data.view = camera->getView();
		scene_data.projection = camera->getProjection();
	}

	scene_uniform->upload(&scene_data, sizeof(GpuSceneData));
}

void RenderScene::updateMeshTransforms()
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

	auto& root = scene->getRoot();
	for (auto* child : root.getChildren())
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

	updateCameraTransform();
	updateMeshTransforms();
}

void RenderScene::rebuild()
{
	context->getLogicalDevice().waitIdle();

	clear();

	if (!world || !world->getActiveScene())
		return;

	auto materials = world->getActiveScene()->getResources<Material>();
	auto submeshes = world->getActiveScene()->getResources<SubMesh>();

	if (materials.size() > material_pool->getSets().size()) {
		uint32_t                            material_count = std::max(1u, static_cast<uint32_t>(materials.size()));
		std::vector<vk::DescriptorPoolSize> material_pool_sizes = {
		    {vk::DescriptorType::eUniformBuffer, material_count},
		    {vk::DescriptorType::eCombinedImageSampler, material_count},
		};
		material_pool = std::make_unique<DescriptorPool>(*context, material_count, material_pool_sizes);
	}

	if (submeshes.size() > object_pool->getSets().size()) {
		uint32_t                            mesh_count = std::max(1u, static_cast<uint32_t>(submeshes.size()));
		std::vector<vk::DescriptorPoolSize> object_pool_sizes = {
		    {vk::DescriptorType::eUniformBuffer, mesh_count * 2},
		};
		object_pool = std::make_unique<DescriptorPool>(*context, mesh_count * 2, object_pool_sizes);
	}

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
	    scene_descriptor->get(),
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

const World* RenderScene::getWorld() const
{
	return world;
}
