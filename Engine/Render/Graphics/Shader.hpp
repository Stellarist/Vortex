#pragma once

#include <unordered_map>

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

class Shader {
private:
	vk::ShaderModule shader;

	std::string            name;
	std::vector<std::byte> codes;

	std::unordered_map<vk::ShaderStageFlagBits, std::string> stages;

	Context* context{};

public:
	Shader(Context& context, std::string_view filename);

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	Shader(Shader&&) noexcept = default;
	Shader& operator=(Shader&&) noexcept = default;

	~Shader();

	void read();
	void create();

	void                                           setStage(vk::ShaderStageFlagBits stage, std::string_view entry);
	vk::PipelineShaderStageCreateInfo              getStage(vk::ShaderStageFlagBits stage) const;
	std::vector<vk::PipelineShaderStageCreateInfo> getStages() const;

	vk::ShaderModule get() const;
};
