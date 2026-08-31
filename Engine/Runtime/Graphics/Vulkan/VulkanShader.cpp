module Runtime.Graphics;

import vulkan;

namespace Vortex {

RHIRef<RHIShader> VulkanDevice::createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode)
{
	validateRHIShaderDesc(desc, bytecode);

	vk::ShaderModuleCreateInfo shader_info{};
	shader_info.setCodeSize(bytecode.size())
	    .setPCode(reinterpret_cast<const uint32*>(bytecode.data()));

	auto shader = makeRHIRef<VulkanShader>(*this, desc);
	shader->shader = device.createShaderModule(shader_info);
	shader->stage_flags = toVkShaderStageFlagBits(desc.type);
	return shader;
}

void VulkanDevice::destroyShader(VulkanShader* shader) noexcept
{
	if (shader->shader)
		device.destroyShaderModule(shader->shader);
	shader->shader = vk::ShaderModule{};
}

}        // namespace Vortex
