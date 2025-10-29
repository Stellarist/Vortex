#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"

class Sampler {
	vk::Sampler sampler;

	Context* context{};

public:
	Sampler(Context& context);

	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;

	Sampler(Sampler&&) noexcept = default;
	Sampler& operator=(Sampler&&) noexcept = default;

	~Sampler();

	void create();

	vk::Sampler get() const;

	// move to shader
	static vk::DescriptorSetLayoutBinding binding(uint32_t binding = {});
};
