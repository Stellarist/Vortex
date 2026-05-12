#pragma once

#include <unordered_map>

#include <vulkan/vulkan.hpp>

#include "VulkanContext.hpp"

class VulkanShader {
private:
	vk::ShaderModule shader;

	std::string          name;
	std::vector<uint8_t> codes;

	std::unordered_map<vk::ShaderStageFlagBits, std::string> stages;

	VulkanContext* context{};

public:
	VulkanShader(VulkanContext& context, std::string_view filename,
	    std::string_view vertex_entry = "vertexMain",
	    std::string_view fragment_entry = "fragmentMain");
	~VulkanShader();

	VulkanShader(const VulkanShader&) = delete;
	VulkanShader& operator=(const VulkanShader&) = delete;

	VulkanShader(VulkanShader&&) noexcept = default;
	VulkanShader& operator=(VulkanShader&&) noexcept = default;

	void read();
	void create();

	void                              setStage(vk::ShaderStageFlagBits stage, std::string_view entry);
	vk::PipelineShaderStageCreateInfo getStage(vk::ShaderStageFlagBits stage) const;

	std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;

	vk::ShaderModule get() const;
};
