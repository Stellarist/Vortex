#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "GpuData.hpp"
#include "Render/Graphics/Buffer.hpp"
#include "Render/Graphics/Descriptor.hpp"
#include "Render/Graphics/Image.hpp"
#include "Scene/Resources/Material.hpp"

class GpuMaterial {
private:
	std::unique_ptr<DescriptorSet> material_descriptor;

	std::unique_ptr<Buffer> material_uniform;
	GpuMaterialData         material_data;

	Image* base_color_texture{};

	std::shared_ptr<Material> source_material{};
	Context*                  context{};

public:
	GpuMaterial(Context&                  context,
	            std::shared_ptr<Material> material,
	            DescriptorSetLayout&      layout,
	            DescriptorPool&           pool,
	            Image*                    default_texture = nullptr);
	~GpuMaterial() = default;

	GpuMaterial(const GpuMaterial&) = delete;
	GpuMaterial& operator=(const GpuMaterial&) = delete;

	GpuMaterial(GpuMaterial&&) noexcept = default;
	GpuMaterial& operator=(GpuMaterial&&) noexcept = default;

	void updateUniforms();
	void bind(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout);

	const DescriptorSet*      getDescriptor();
	std::shared_ptr<Material> getSourceMaterial() const;
};
