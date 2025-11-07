#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "Render/Graphics/Context.hpp"
#include "Render/Graphics/Image.hpp"
#include "Render/Graphics/Sampler.hpp"
#include "Scene/Resources/Texture.hpp"

class GpuTexture {
private:
	std::unique_ptr<Image>   image;
	std::shared_ptr<Sampler> sampler;

	std::shared_ptr<Texture> source_texture;
	Context*                 context{};

	void createFromMemory(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t channels);

public:
	GpuTexture(Context& context, std::shared_ptr<Texture> texture, std::shared_ptr<Sampler> sampler = nullptr);
	~GpuTexture() = default;

	GpuTexture(const GpuTexture&) = delete;
	GpuTexture& operator=(const GpuTexture&) = delete;

	GpuTexture(GpuTexture&&) noexcept = default;
	GpuTexture& operator=(GpuTexture&&) noexcept = default;

	Image*                   getImage() const;
	Sampler*                 getSampler() const;
	std::shared_ptr<Texture> getSourceTexture() const;
};
