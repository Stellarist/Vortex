export module Runtime.Graphics:Vulkan.Shader;

import vulkan;
import Core;
import :Vulkan.Device;
import :RHI.Shader;

export namespace Vortex {

class VulkanShader : public RHIShader {
private:
	RHIShaderDesc desc{};

	vk::ShaderModule        shader{};
	vk::ShaderStageFlagBits stage_flags{};

	VulkanDevice& device;

	friend class VulkanDevice;

protected:
	void applyName(const std::string& name) noexcept override
	{
		device.setName(shader, name);
	}

public:
	VulkanShader(VulkanDevice& device, RHIShaderDesc desc) : device(device), desc(std::move(desc)) {}
	~VulkanShader() override { device.destroyShader(this); }

	const RHIShaderDesc& getDesc() const noexcept override { return desc; }

	vk::ShaderModule getHandle() const noexcept { return shader; }
	vk::ShaderStageFlagBits getStage() const noexcept { return stage_flags; }
};

}        // namespace Vortex
