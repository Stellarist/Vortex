#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "Runtime/Render/Backend/VulkanContext.hpp"
#include "Runtime/Render/Backend/VulkanImage.hpp"
#include "Runtime/Render/Backend/VulkanSampler.hpp"
#include "Runtime/World/Resources/Texture.hpp"

class GpuTexture {
private:
	std::unique_ptr<VulkanImage>   image;
	std::shared_ptr<VulkanSampler> sampler;

	std::shared_ptr<Texture> source_texture;
	VulkanContext*                 context{};

	void createFromMemory(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t channels);

public:
	GpuTexture(VulkanContext& context, std::shared_ptr<Texture> texture, std::shared_ptr<VulkanSampler> sampler = nullptr);
	~GpuTexture() = default;

	GpuTexture(const GpuTexture&) = delete;
	GpuTexture& operator=(const GpuTexture&) = delete;

	GpuTexture(GpuTexture&&) noexcept = default;
	GpuTexture& operator=(GpuTexture&&) noexcept = default;

	VulkanImage*                   getImage() const;
	VulkanSampler*                 getSampler() const;
	std::shared_ptr<Texture> getSourceTexture() const;
};
