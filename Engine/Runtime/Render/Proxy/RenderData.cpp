#include "RenderData.hpp"

#include "Runtime/World/Base/Node.hpp"

RHIVertexBindingDesc RenderVertex::binding(uint32_t binding)
{
	return {
	    binding,
	    sizeof(RenderVertex),
	    false,
	};
}

std::vector<RHIVertexAttributeDesc> RenderVertex::attributes(uint32_t binding)
{
	return {
	    {"POSITION", RHIFormat::RGB32_FLOAT, 0, binding, offsetof(RenderVertex, pos)},
	    {"NORMAL", RHIFormat::RGB32_FLOAT, 1, binding, offsetof(RenderVertex, normal)},
	    {"TEXCOORD", RHIFormat::RG32_FLOAT, 2, binding, offsetof(RenderVertex, uv)},
	    {"COLOR", RHIFormat::RGBA32_FLOAT, 3, binding, offsetof(RenderVertex, color)},
	};
}


RenderCameraData RenderCameraData::convert(const Camera& camera)
{
	RenderCameraData data{};

	data.view = camera.getView();
	data.projection = camera.getProjection();
	data.position = glm::vec4(glm::inverse(camera.getView())[3]);

	return data;
}


RenderLightData RenderLightData::convert(const Light& light)
{
	RenderLightData data{};

	glm::mat4 world_matrix = light.getNode()->getTransform().getWorldMatrix();
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
		data.position = glm::vec4(position, 2.0f);
		data.direction = glm::vec4(direction, 0.0f);
		data.color = glm::vec4(spot_light->getColor(), spot_light->getIntensity());
		data.params = glm::vec4(spot_light->getRange(), spot_light->getInnerConeAngle(), spot_light->getOuterConeAngle(), 2.0f);
	}

	return data;
}


RHIDescriptorLayoutDesc RenderObjectData::layout(uint32_t binding)
{
	RHIDescriptorLayoutItem item{};
	item.setSlot(binding).setType(RHIDescriptorType::UniformBuffer);

	RHIDescriptorLayoutDesc desc{};
	desc.setVisibility(RHIShaderType::Vertex).addBindingItem(item);

	return desc;
}


RHIDescriptorLayoutDesc RenderMaterialData::layout(uint32_t binding)
{
	RHIDescriptorLayoutItem item0{};
	item0.setSlot(binding + 0).setType(RHIDescriptorType::UniformBuffer);

	RHIDescriptorLayoutItem item1{};
	item1.setSlot(binding + 1).setType(RHIDescriptorType::TextureSRV);

	RHIDescriptorLayoutItem item2{};
	item2.setSlot(binding + 2).setType(RHIDescriptorType::TextureSRV);

	RHIDescriptorLayoutItem item3{};
	item3.setSlot(binding + 3).setType(RHIDescriptorType::Sampler);

	RHIDescriptorLayoutDesc desc{};
	desc.setVisibility(RHIShaderType::Fragment)
	    .addBindingItem(item0)
	    .addBindingItem(item1)
	    .addBindingItem(item2)
	    .addBindingItem(item3);

	return desc;
}


RHIDescriptorLayoutDesc RenderSceneData::layout(uint32_t binding)
{
	RHIDescriptorLayoutItem item{};
	item.setSlot(binding).setType(RHIDescriptorType::UniformBuffer);

	RHIDescriptorLayoutDesc desc{};
	desc.setVisibility(RHIShaderType::Vertex | RHIShaderType::Fragment).addBindingItem(item);

	return desc;
}
