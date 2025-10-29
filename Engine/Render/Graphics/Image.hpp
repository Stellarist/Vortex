#pragma once

#include <vulkan/vulkan.hpp>

#include "Context.hpp"
#include "Buffer.hpp"
#include "Sampler.hpp"

class Image {
private:
	vk::Image        image;
	vk::ImageView    view;
	vk::DeviceMemory memory;

	int   width{};
	int   height{};
	int   channels{};
	void* data;

	std::unique_ptr<Buffer> buffer;

	Context* context{};
	Sampler* sampler{};

	uint32_t queryMemoryType(uint32_t type, vk::MemoryPropertyFlags prop_flags) const;

public:
	Image(Context& context, std::string_view file_path);

	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;

	Image(Image&&) noexcept = default;
	Image& operator=(Image&&) noexcept = default;

	~Image();

	void readImage(std::string_view file_path);
	void freeImage();
	void createBuffer(uint32_t image_size);
	void createImage(uint32_t width, uint32_t height);
	void allocateMemory();
	void createImageView();
	void copyBufferToImage(vk::CommandBuffer command, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
	void transitionImageLayout(vk::CommandBuffer command, vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

	vk::Image     get() const;
	vk::ImageView getView() const;

	void     setSampler(Sampler& sampler);
	Sampler& getSampler() const;
};
