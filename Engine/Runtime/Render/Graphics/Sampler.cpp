#include "Sampler.hpp"

#include "Device.hpp"

Sampler::Sampler(Context& context) :
    context(&context)
{
	create();
}

Sampler::~Sampler()
{
	context->getDevice().logical().destroySampler(sampler);
}

void Sampler::create()
{
	vk::SamplerCreateInfo create_info{};
	create_info.setMagFilter(vk::Filter::eLinear)
	    .setMinFilter(vk::Filter::eLinear)
	    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
	    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
	    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
	    .setAnisotropyEnable(vk::False)
	    .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
	    .setUnnormalizedCoordinates(vk::False)
	    .setCompareEnable(vk::False)
	    .setCompareOp(vk::CompareOp::eAlways)
	    .setMipmapMode(vk::SamplerMipmapMode::eLinear);

	sampler = context->getDevice().logical().createSampler(create_info);
}

vk::Sampler Sampler::get() const
{
	return sampler;
}

vk::DescriptorSetLayoutBinding Sampler::binding(uint32_t binding)
{
	vk::DescriptorSetLayoutBinding layout_binding{};
	layout_binding.setBinding(binding)
	    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
	    .setDescriptorCount(1)
	    .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	return layout_binding;
}
