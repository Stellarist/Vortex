#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "GpuMesh.hpp"
#include "GpuTexture.hpp"
#include "Runtime/Render/Backend/VulkanBuffer.hpp"
#include "Runtime/Render/Backend/VulkanContext.hpp"
#include "Runtime/Render/Backend/VulkanDescriptor.hpp"
#include "Runtime/Render/Resources/GpuMaterial.hpp"
#include "Runtime/World/World.hpp"
#include "Runtime/World/Resources/Texture.hpp"

class GpuScene {
private:
	const World* world{};

	// Set 0: Scene-level descriptor
	VulkanDescriptorSet                        scene_descriptor;
	std::unique_ptr<VulkanDescriptorSetLayout> scene_layout;
	std::unique_ptr<DescriptorPool>      scene_pool;
	std::unique_ptr<VulkanBuffer>        scene_uniform;
	GpuSceneData                         scene_data;

	// Set 1: Material-level
	std::unique_ptr<VulkanDescriptorSetLayout>      material_layout;
	std::unique_ptr<DescriptorPool>           material_pool;
	std::shared_ptr<VulkanSampler>                  default_sampler;
	std::vector<std::unique_ptr<GpuMaterial>> gpu_materials;
	std::vector<std::unique_ptr<GpuTexture>>  gpu_textures;

	std::unordered_map<std::shared_ptr<Texture>, GpuTexture*> texture_to_gpu_texture;

	// Set 2: Object-level
	std::unique_ptr<VulkanDescriptorSetLayout>  object_layout;
	std::unique_ptr<DescriptorPool>       object_pool;
	std::vector<std::unique_ptr<GpuMesh>> gpu_meshes;

	std::unordered_map<std::shared_ptr<Material>, std::vector<GpuMesh*>> meshes_by_material;
	std::unordered_map<std::shared_ptr<SubMesh>, GpuMesh*>               submesh_to_gpu_mesh;

	size_t last_submesh_count{0};
	size_t last_texture_count{0};
	size_t last_material_count{0};

	VulkanContext* context{};

	void createDescriptorLayouts();
	void createDescriptorPools();
	void createSceneDescriptor();

	void updateCamera();
	void updateLights();
	void updateMesh();

	void loadTextures();
	void loadMaterials();
	void loadMeshes();
	void organizeMeshesByMaterial();

	bool needsRebuild() const;
	void clear();

public:
	GpuScene() = default;
	GpuScene(VulkanContext& context, const World& world);
	~GpuScene() = default;

	GpuScene(const GpuScene&) = delete;
	GpuScene& operator=(const GpuScene&) = delete;

	GpuScene(GpuScene&&) noexcept = default;
	GpuScene& operator=(GpuScene&&) noexcept = default;

	void update(float dt);
	void rebuild();
	void draw(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout);

	std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts() const;

	VulkanDescriptorSet        getSceneDescriptor();
	VulkanDescriptorSetLayout* getSceneLayout();
	VulkanDescriptorSetLayout* getMaterialLayout();
	VulkanDescriptorSetLayout* getObjectLayout();

	const World* getWorld() const;
};
