#include "Shader.hpp"

#include <fstream>

Shader::Shader(Context& context, std::string_view filename) :
    context(&context), name(filename)
{
	read();
	create();
}

Shader::~Shader()
{
	context->getLogicalDevice().destroyShaderModule(shader);
}

void Shader::read()
{
	std::ifstream file(std::string(name), std::ios::binary | std::ios::ate);
	if (!file.is_open())
		throw std::runtime_error("Failed to open shader file: " + name);

	codes.resize(file.tellg());
	file.seekg(0);
	file.read(reinterpret_cast<char*>(codes.data()), codes.size());
	if (file.fail())
		throw std::runtime_error("Failed to read shader file: " + name);
}

void Shader::create()
{
	vk::ShaderModuleCreateInfo create_info{};
	create_info.setCodeSize(codes.size())
	    .setPCode(reinterpret_cast<const uint32_t*>(codes.data()));
	shader = context->getLogicalDevice().createShaderModule(create_info);
	if (!shader)
		throw std::runtime_error("Failed to create shader module: " + name);
}

void Shader::setStage(vk::ShaderStageFlagBits stage, std::string_view entry)
{
	if (stages.contains(stage))
		throw std::runtime_error("Shader stage already exists: " + std::to_string(static_cast<int>(stage)));

	stages[stage] = entry;
}

vk::PipelineShaderStageCreateInfo Shader::getStage(vk::ShaderStageFlagBits stage) const
{
	auto it = stages.find(stage);
	if (it == stages.end())
		throw std::runtime_error("Shader stage not found: " + std::to_string(static_cast<int>(stage)));

	vk::PipelineShaderStageCreateInfo stage_info{};
	stage_info.setStage(stage)
	    .setModule(shader)
	    .setPName(it->second.c_str());

	return stage_info;
}

std::vector<vk::PipelineShaderStageCreateInfo> Shader::getStages() const
{
	std::vector<vk::PipelineShaderStageCreateInfo> stage_infos;
	stage_infos.reserve(stages.size());

	for (const auto& [stage, entry] : stages)
		stage_infos.push_back(getStage(stage));

	return stage_infos;
}

vk::ShaderModule Shader::get() const
{
	return shader;
}
