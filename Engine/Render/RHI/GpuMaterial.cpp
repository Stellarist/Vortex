#include "GpuMaterial.hpp"

GpuMaterial::GpuMaterial(Context&                  context,
                         std::shared_ptr<Material> material,
                         DescriptorSetLayout&      layout,
                         DescriptorPool&           pool,
                         Image*                    default_texture) :
    source_material(material),
    context(&context)
{
	if (auto* pbr = dynamic_cast<const PBRMaterial*>(material.get())) {
		material_data.base_color = pbr->getBaseColorFactor();
		material_data.metallic = pbr->getMetallicFactor();
		material_data.roughness = pbr->getRoughnessFactor();
	}

	// Create material uniform buffer
	material_uniform = Buffer::createDynamic(
	    context,
	    vk::BufferUsageFlagBits::eUniformBuffer,
	    &material_data,
	    sizeof(GpuMaterialData));

	auto textures = material->getTextures();
	if (textures.find("baseColor") != textures.end())
		// TODO: Convert scene::Texture to render::Image
		base_color_texture = default_texture;
	else
		base_color_texture = default_texture;

	auto set_index = pool.allocate(layout);
	material_descriptor = std::make_unique<DescriptorSet>(context, pool.getSet(set_index).get());

	material_descriptor->update(0, vk::DescriptorType::eUniformBuffer, material_uniform.get());
	if (base_color_texture)
		material_descriptor->update(1, vk::DescriptorType::eCombinedImageSampler, base_color_texture);
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
	    material_descriptor->get(),
	    {});
}

const DescriptorSet* GpuMaterial::getDescriptor()
{
	return material_descriptor.get();
}

std::shared_ptr<Material> GpuMaterial::getSourceMaterial() const
{
	return source_material;
}
