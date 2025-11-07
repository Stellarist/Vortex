#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

constexpr uint32_t MAX_LIGHTS = 16;

struct GpuVertex {
	glm::vec3 pos{0.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec2 uv{0.0f};
	glm::vec4 color{1.0f};

	static vk::VertexInputBindingDescription                binding(uint32_t binding = {});
	static std::vector<vk::VertexInputAttributeDescription> attributes(uint32_t binding = {});
};

struct GpuObjectData {
	glm::mat4 model{1.0f};

	static vk::DescriptorSetLayoutBinding binding(uint32_t binding = 0);
};

struct GpuLightData {
	glm::vec4 position;
	glm::vec4 direction;
	glm::vec4 color;
	glm::vec4 params;
};

struct GpuMaterialData {
	glm::vec4 base_color{1.0f};
	float     metallic{0.0f};
	float     roughness{1.0f};
	float     padding1;
	float     padding2;

	static std::vector<vk::DescriptorSetLayoutBinding> bindings(uint32_t base_binding = 0);
};

struct GpuSceneData {
	glm::mat4    view{1.0f};
	glm::mat4    projection{1.0f};
	glm::vec4    ambient_color{0.1f, 0.1f, 0.1f, 1.0f};
	glm::vec4    camera_position{0.0f, 0.0f, 0.0f, 1.0f};
	uint32_t     light_count{0};
	float        padding1;
	float        padding2;
	float        padding3;
	GpuLightData lights[MAX_LIGHTS];

	static vk::DescriptorSetLayoutBinding binding(uint32_t binding = 0);
};
