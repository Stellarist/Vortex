#include "Shader.hpp"

#include "Device.hpp"
#include "Core/File/FileSystem.hpp"

Shader::Shader(Context& context, std::string_view filename,
    std::string_view vertex_entry,
    std::string_view fragment_entry) :
    context(&context), name(filename)
{
	read();
	create();

	setStage(vk::ShaderStageFlagBits::eVertex, vertex_entry);
	setStage(vk::ShaderStageFlagBits::eFragment, fragment_entry);
}

Shader::~Shader()
{
	context->getDevice().logical().destroyShaderModule(shader);
}

void Shader::read()
{
	codes = FileSystem::readBinaryFile(std::string(name));
}

void Shader::create()
{
	vk::ShaderModuleCreateInfo create_info{};
	create_info.setCodeSize(codes.size())
	    .setPCode(reinterpret_cast<const uint32_t*>(codes.data()));

	shader = context->getDevice().logical().createShaderModule(create_info);
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
