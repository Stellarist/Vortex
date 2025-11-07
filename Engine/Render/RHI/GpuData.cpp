#include "GpuData.hpp"

vk::VertexInputBindingDescription GpuVertex::binding(uint32_t binding)
{
	return {
	    binding,
	    sizeof(GpuVertex),
	    vk::VertexInputRate::eVertex,
	};
}

std::vector<vk::VertexInputAttributeDescription> GpuVertex::attributes(uint32_t binding)
{
	return {
	    {
	        0,
	        binding,
	        vk::Format::eR32G32B32Sfloat,
	        offsetof(GpuVertex, pos),
	    },
	    {
	        1,
	        binding,
	        vk::Format::eR32G32B32Sfloat,
	        offsetof(GpuVertex, normal),
	    },
	    {
	        2,
	        binding,
	        vk::Format::eR32G32Sfloat,
	        offsetof(GpuVertex, uv),
	    },
	    {
	        3,
	        binding,
	        vk::Format::eR32G32B32A32Sfloat,
	        offsetof(GpuVertex, color),
	    },
	};
}

vk::DescriptorSetLayoutBinding GpuSceneData::binding(uint32_t binding)
{
	return {
	    binding,
	    vk::DescriptorType::eUniformBuffer,
	    1,
	    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
	};
}

std::vector<vk::DescriptorSetLayoutBinding> GpuMaterialData::bindings(uint32_t base_binding)
{
	return {
	    {
	        base_binding + 0,
	        vk::DescriptorType::eUniformBuffer,
	        1,
	        vk::ShaderStageFlagBits::eFragment,
	    },
	    {
	        base_binding + 1,
	        vk::DescriptorType::eCombinedImageSampler,
	        1,
	        vk::ShaderStageFlagBits::eFragment,
	    },
	    {
	        base_binding + 2,
	        vk::DescriptorType::eCombinedImageSampler,
	        1,
	        vk::ShaderStageFlagBits::eFragment,
	    },
	    {
	        base_binding + 3,
	        vk::DescriptorType::eCombinedImageSampler,
	        1,
	        vk::ShaderStageFlagBits::eFragment,
	    },
	};
}

vk::DescriptorSetLayoutBinding GpuObjectData::binding(uint32_t binding)
{
	return {
	    binding,
	    vk::DescriptorType::eUniformBuffer,
	    1,
	    vk::ShaderStageFlagBits::eVertex,
	};
}
