#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

class Sampler {
	vk::Sampler sampler;

	Context* context{};

public:
	Sampler(Context& context);
	~Sampler();

	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;

	Sampler(Sampler&&) noexcept = default;
	Sampler& operator=(Sampler&&) noexcept = default;

	void create();

	vk::Sampler get() const;

	static vk::DescriptorSetLayoutBinding binding(uint32_t binding = {});
};
