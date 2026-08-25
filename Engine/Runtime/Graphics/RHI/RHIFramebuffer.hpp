export module Runtime.Graphics:RHI.Framebuffer;

import Core;
import :RHI.Texture;

export namespace Vortex {

struct RHIFramebufferAttachment {
	RHIRef<RHITextureView> texture_view{};

	RHIFormat format{RHIFormat::Unknown};
	bool      read_only{false};

	bool valid() const noexcept
	{
		return static_cast<bool>(texture_view);
	}

	RHIFramebufferAttachment& setTextureView(RHITextureView* new_texture_view) noexcept
	{
		texture_view = new_texture_view;
		return *this;
	}

	RHIFramebufferAttachment& setFormat(RHIFormat new_format) noexcept
	{
		format = new_format;
		return *this;
	}

	RHIFramebufferAttachment& setReadOnly(bool new_read_only) noexcept
	{
		read_only = new_read_only;
		return *this;
	}
};

struct RHIFramebufferDesc {
	uint32 width{};
	uint32 height{};
	uint32 array_size{1};
	uint32 sample_count{1};
	uint32 sample_quality{};

	std::vector<RHIFramebufferAttachment> color_attachments{};
	RHIFramebufferAttachment              depth_attachment{};

	std::vector<RHIFormat> color_formats{};
	RHIFormat              depth_format{RHIFormat::Unknown};

	RHIFramebufferDesc& setWidth(uint32 new_width) noexcept
	{
		width = new_width;
		return *this;
	}

	RHIFramebufferDesc& setHeight(uint32 new_height) noexcept
	{
		height = new_height;
		return *this;
	}

	RHIFramebufferDesc& setArraySize(uint32 new_array_size) noexcept
	{
		array_size = new_array_size;
		return *this;
	}

	RHIFramebufferDesc& setSampleCount(uint32 new_sample_count) noexcept
	{
		sample_count = new_sample_count;
		return *this;
	}

	RHIFramebufferDesc& setSampleQuality(uint32 new_sample_quality) noexcept
	{
		sample_quality = new_sample_quality;
		return *this;
	}

	RHIFramebufferDesc& addColorAttachment(const RHIFramebufferAttachment& attachment)
	{
		color_attachments.push_back(attachment);
		return *this;
	}

	RHIFramebufferDesc& addColorAttachment(RHITextureView* texture_view)
	{
		color_attachments.push_back(RHIFramebufferAttachment().setTextureView(texture_view));
		return *this;
	}

	RHIFramebufferDesc& setDepthAttachment(const RHIFramebufferAttachment& attachment) noexcept
	{
		depth_attachment = attachment;
		return *this;
	}

	RHIFramebufferDesc& setDepthAttachment(RHITextureView* texture_view) noexcept
	{
		depth_attachment = RHIFramebufferAttachment().setTextureView(texture_view);
		return *this;
	}
};

class RHIFramebuffer : public RHIResource {
public:
	virtual const RHIFramebufferDesc& getDesc() const noexcept = 0;
};

}        // namespace Vortex
