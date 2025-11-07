#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "Context.hpp"
#include "Image.hpp"
#include "Sampler.hpp"

enum class GBufferAttachment {
	Position = 0,
	Normal,
	Albedo,
	Metallic,
	Roughness,
	Depth,
	Count
};

class GBuffer {
private:
	uint32_t width;
	uint32_t height;

	std::unique_ptr<Sampler> sampler;

	std::unordered_map<GBufferAttachment, std::unique_ptr<Image>>                     attachments;
	std::unordered_map<GBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos;

	Context* context{};

	void createAttachments();

public:
	GBuffer(Context& ctx, uint32_t width, uint32_t height,
	    std::unordered_map<GBufferAttachment, std::pair<vk::Format, vk::ImageUsageFlags>> attachment_infos);
	~GBuffer() = default;

	GBuffer(const GBuffer&) = delete;
	GBuffer& operator=(const GBuffer&) = delete;

	GBuffer(GBuffer&&) noexcept = default;
	GBuffer& operator=(GBuffer&&) noexcept = default;

	void resize(uint32_t width, uint32_t height);

	Image*        getImage(GBufferAttachment attachment) const;
	vk::ImageView getImageView(GBufferAttachment attachment) const;
};
