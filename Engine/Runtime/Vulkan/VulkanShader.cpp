module Runtime.Vulkan;

import vulkan;

namespace Vortex {

RHIRef<RHIShader> VulkanDevice::createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode)
{
	validateRHIShaderDesc(desc, bytecode);

	auto shader = makeRHIRef<VulkanShader>(desc, bytecode);
	shader->stage_flags = toVkShaderStageFlagBits(desc.type);
	return shader;
}

}        // namespace Vortex
