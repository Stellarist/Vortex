#include "GpuData.hpp"

#include "Scene/Core/Node.hpp"

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

GpuCameraData GpuCameraData::convert(const Camera& camera)
{
	GpuCameraData data;
	data.view = camera.getView();
	data.projection = camera.getProjection();
	data.position = glm::vec4(glm::inverse(data.view)[3]);
	return data;
}

GpuLightData GpuLightData::convert(const Light& light)
{
	GpuLightData data;

	auto world_matrix = light.getNode()->getTransform().getWorldMatrix();

	glm::vec3 position = glm::vec3(world_matrix[3]);
	glm::vec3 direction = glm::normalize(glm::vec3(world_matrix * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));

	if (const auto* dir_light = dynamic_cast<const DirectionalLight*>(&light)) {
		data.position = glm::vec4(direction, 0.0f);
		data.direction = glm::vec4(direction, 0.0f);
		data.color = glm::vec4(dir_light->getColor(), dir_light->getIntensity());
		data.params = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	} else if (const auto* point_light = dynamic_cast<const PointLight*>(&light)) {
		data.position = glm::vec4(position, 1.0f);
		data.direction = glm::vec4(0.0f);
		data.color = glm::vec4(point_light->getColor(), point_light->getIntensity());
		data.params = glm::vec4(point_light->getRange(), 0.0f, 0.0f, 1.0f);

	} else if (const auto* spot_light = dynamic_cast<const SpotLight*>(&light)) {
		data.position = glm::vec4(position, 1.0f);
		data.direction = glm::vec4(direction, 0.0f);
		data.color = glm::vec4(spot_light->getColor(), spot_light->getIntensity());
		data.params = glm::vec4(spot_light->getRange(), spot_light->getInnerConeAngle(), spot_light->getOuterConeAngle(), 2.0f);
	}

	return data;
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
