#include "GpuMaterial.hpp"

GpuMaterial::GpuMaterial(Context& context,
    std::shared_ptr<Material>     material,
    DescriptorSetLayout&          layout,
    DescriptorPool&               pool,
    Image*                        base_color,
    Image*                        metallic_roughness) :
    source_material(material),
    context(&context),
    base_color_texture(base_color),
    metallic_roughness_texture(metallic_roughness)
{
	if (auto* pbr = dynamic_cast<const PBRMaterial*>(material.get())) {
		material_data.base_color = pbr->getBaseColorFactor();
		material_data.metallic = pbr->getMetallicFactor();
		material_data.roughness = pbr->getRoughnessFactor();
	}

	material_uniform = Buffer::createDynamic(
	    context,
	    vk::BufferUsageFlagBits::eUniformBuffer,
	    &material_data,
	    sizeof(GpuMaterialData));

	material_descriptor = pool.allocate(layout);

	auto& device = context.getDevice();
	material_descriptor.update(device, 0, vk::DescriptorType::eUniformBuffer, material_uniform.get());

	if (base_color_texture)
		material_descriptor.update(device, 1, vk::DescriptorType::eCombinedImageSampler, base_color_texture);

	if (metallic_roughness_texture)
		material_descriptor.update(device, 2, vk::DescriptorType::eCombinedImageSampler, metallic_roughness_texture);
}

void GpuMaterial::updateUniforms()
{
	material_uniform->upload(&material_data, sizeof(GpuMaterialData));
}

void GpuMaterial::bind(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout)
{
	command_buffer.bindDescriptorSets(
	    vk::PipelineBindPoint::eGraphics,
	    pipeline_layout,
	    1,
	    material_descriptor.get(),
	    {});
}

DescriptorSet GpuMaterial::getDescriptor()
{
	return material_descriptor.get();
}

std::shared_ptr<Material> GpuMaterial::getSourceMaterial() const
{
	return source_material;
}
