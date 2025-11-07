#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "GpuMesh.hpp"
#include "GpuTexture.hpp"
#include "Render/Graphics/Buffer.hpp"
#include "Render/Graphics/Context.hpp"
#include "Render/Graphics/Descriptor.hpp"
#include "Render/RHI/GpuMaterial.hpp"
#include "Scene/World.hpp"
#include "Scene/Resources/Texture.hpp"

class RenderScene {
private:
	const World* world{};

	// Set 0: Scene-level descriptor
	DescriptorSet                        scene_descriptor;
	std::unique_ptr<DescriptorSetLayout> scene_layout;
	std::unique_ptr<DescriptorPool>      scene_pool;
	std::unique_ptr<Buffer>              scene_uniform;
	GpuSceneData                         scene_data;

	// Set 1: Material-level
	std::unique_ptr<DescriptorSetLayout>      material_layout;
	std::unique_ptr<DescriptorPool>           material_pool;
	std::shared_ptr<Sampler>                  default_sampler;
	std::vector<std::unique_ptr<GpuMaterial>> gpu_materials;
	std::vector<std::unique_ptr<GpuTexture>>  gpu_textures;

	std::unordered_map<std::shared_ptr<Texture>, GpuTexture*> texture_to_gpu_texture;

	// Set 2: Object-level
	std::unique_ptr<DescriptorSetLayout>  object_layout;
	std::unique_ptr<DescriptorPool>       object_pool;
	std::vector<std::unique_ptr<GpuMesh>> gpu_meshes;

	std::unordered_map<std::shared_ptr<Material>, std::vector<GpuMesh*>> meshes_by_material;
	std::unordered_map<std::shared_ptr<SubMesh>, GpuMesh*>               submesh_to_gpu_mesh;

	size_t last_submesh_count{0};
	size_t last_material_count{0};

	Context* context{};

	void createDescriptorLayouts();
	void createDescriptorPools();
	void createSceneDescriptor();

	void loadTextures();
	void loadMaterials();
	void loadMeshes();
	void organizeMeshesByMaterial();

	bool needsRebuild() const;
	void clear();

	void updateCamera();
	void updateLights();
	void updateMesh();

	glm::mat4 getWorldMatrix(const Node* node) const;

public:
	RenderScene() = default;
	RenderScene(Context& context, const World& world);
	~RenderScene() = default;

	RenderScene(const RenderScene&) = delete;
	RenderScene& operator=(const RenderScene&) = delete;

	RenderScene(RenderScene&&) noexcept = default;
	RenderScene& operator=(RenderScene&&) noexcept = default;

	void update(float dt);
	void rebuild();
	void draw(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout);

	std::vector<vk::DescriptorSetLayout> getDescriptorSetLayouts() const;

	DescriptorSet        getSceneDescriptor();
	DescriptorSetLayout* getSceneLayout();
	DescriptorSetLayout* getMaterialLayout();
	DescriptorSetLayout* getObjectLayout();

	const World* getWorld() const;
};
