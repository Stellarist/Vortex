#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "GpuData.hpp"
#include "Runtime/Render/Backend/VulkanBuffer.hpp"
#include "Runtime/Render/Backend/VulkanDescriptor.hpp"
#include "Runtime/Render/Backend/VulkanImage.hpp"
#include "Runtime/World/Resources/Material.hpp"

class GpuMaterial {
private:
	VulkanDescriptorSet material_descriptor;

	std::unique_ptr<VulkanBuffer> material_uniform;
	GpuMaterialData               material_data;

	std::shared_ptr<Material> source_material;

	VulkanImage* base_color_texture{};
	VulkanImage* metallic_roughness_texture{};

	VulkanContext* context{};

public:
	GpuMaterial(VulkanContext&          context,
	    std::shared_ptr<Material> material,
	    VulkanDescriptorSetLayout&      layout,
	    DescriptorPool&           pool,
	    VulkanImage*                    base_color = nullptr,
	    VulkanImage*                    metallic_roughness = nullptr);
	~GpuMaterial() = default;

	GpuMaterial(const GpuMaterial&) = delete;
	GpuMaterial& operator=(const GpuMaterial&) = delete;

	GpuMaterial(GpuMaterial&&) noexcept = default;
	GpuMaterial& operator=(GpuMaterial&&) noexcept = default;

	void updateUniforms();
	void bind(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout);

	VulkanDescriptorSet             getDescriptor();
	std::shared_ptr<Material> getSourceMaterial() const;
};
