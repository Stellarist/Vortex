module;

#include <cstddef>

module Runtime.Graphics;

namespace Vortex {

RHIVertexBindingDesc RenderVertex::binding(uint32 binding)
{
	return {
	    binding,
	    sizeof(RenderVertex),
	    false,
	};
}

std::vector<RHIVertexAttributeDesc> RenderVertex::attributes(uint32 binding)
{
	return {
	    {"POSITION", RHIFormat::RGB32_FLOAT, 0, binding, offsetof(RenderVertex, pos)},
	    {"NORMAL", RHIFormat::RGB32_FLOAT, 1, binding, offsetof(RenderVertex, normal)},
	    {"TEXCOORD", RHIFormat::RG32_FLOAT, 2, binding, offsetof(RenderVertex, uv)},
	    {"COLOR", RHIFormat::RGBA32_FLOAT, 3, binding, offsetof(RenderVertex, color)},
	};
}


RenderCameraData RenderCameraData::convert(const CameraComponent& camera)
{
	RenderCameraData data{};

	data.view = camera.getView();
	data.projection = camera.getProjection();
	data.position = Vec4(Math::inverse(camera.getView())[3]);

	return data;
}


RenderLightData RenderLightData::convert(const LightComponent& light)
{
	RenderLightData data{};

	Mat4 world_matrix = light.getWorldMatrix();
	Vec3 position = Vec3(world_matrix[3]);
	Vec3 direction = Math::normalize(Vec3(world_matrix * Vec4(0.0f, -1.0f, 0.0f, 0.0f)));

	if (const auto* dir_light = dynamic_cast<const DirectionalLightComponent*>(&light)) {
		data.position = Vec4(direction, 0.0f);
		data.direction = Vec4(direction, 0.0f);
		data.color = Vec4(dir_light->getColor(), dir_light->getIntensity());
		data.params = Vec4(0.0f, 0.0f, 0.0f, 0.0f);

	} else if (const auto* point_light = dynamic_cast<const PointLightComponent*>(&light)) {
		data.position = Vec4(position, 1.0f);
		data.direction = Vec4(0.0f);
		data.color = Vec4(point_light->getColor(), point_light->getIntensity());
		data.params = Vec4(point_light->getRange(), 0.0f, 0.0f, 1.0f);

	} else if (const auto* spot_light = dynamic_cast<const SpotLightComponent*>(&light)) {
		data.position = Vec4(position, 2.0f);
		data.direction = Vec4(direction, 0.0f);
		data.color = Vec4(spot_light->getColor(), spot_light->getIntensity());
		data.params = Vec4(spot_light->getRange(), spot_light->getInnerConeAngle(), spot_light->getOuterConeAngle(), 2.0f);
	}

	return data;
}


RHIBindingLayoutDesc RenderObjectData::layout(uint32 binding)
{
	RHIBindingLayoutItem item{};
	item.setSlot(binding).setType(RHIBindingType::ConstantBuffer);

	RHIBindingLayoutDesc desc{};
	desc.setVisibility(RHIShaderType::Vertex).addItem(item);

	return desc;
}


RHIBindingLayoutDesc RenderMaterialData::layout(uint32 binding)
{
	RHIBindingLayoutItem item0{};
	item0.setSlot(binding + 0).setType(RHIBindingType::ConstantBuffer);

	RHIBindingLayoutItem item1{};
	item1.setSlot(binding + 1).setType(RHIBindingType::TextureSRV);

	RHIBindingLayoutItem item2{};
	item2.setSlot(binding + 2).setType(RHIBindingType::TextureSRV);

	RHIBindingLayoutItem item3{};
	item3.setSlot(binding + 3).setType(RHIBindingType::Sampler);

	RHIBindingLayoutDesc desc{};
	desc.setVisibility(RHIShaderType::Pixel)
	    .addItem(item0)
	    .addItem(item1)
	    .addItem(item2)
	    .addItem(item3);

	return desc;
}


RHIBindingLayoutDesc RenderSceneData::layout(uint32 binding)
{
	RHIBindingLayoutItem item{};
	item.setSlot(binding).setType(RHIBindingType::ConstantBuffer);

	RHIBindingLayoutDesc desc{};
	desc.setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel).addItem(item);

	return desc;
}

}        // namespace Vortex
