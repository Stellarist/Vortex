export module Runtime.Vulkan:Shader;

import vulkan;
import Core;
import Runtime.RHI;

export namespace Vortex {

class VulkanShader : public RHIShader {
private:
	RHIShaderDesc desc{};

	std::vector<std::byte> bytecode{};
	vk::ShaderStageFlagBits stage_flags{};

	friend class VulkanDevice;

public:
	VulkanShader(RHIShaderDesc desc, std::span<const std::byte> bytecode) :
	    desc(std::move(desc)), bytecode(bytecode.begin(), bytecode.end()) {}

	const RHIShaderDesc& getDesc() const noexcept override { return desc; }

	std::span<const std::byte> getBytecode() const noexcept { return bytecode; }
	vk::ShaderStageFlagBits getStage() const noexcept { return stage_flags; }
};

}        // namespace Vortex
