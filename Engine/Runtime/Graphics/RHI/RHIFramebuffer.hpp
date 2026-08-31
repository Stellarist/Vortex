export module Runtime.Graphics:RHI.Framebuffer;

import Core;
import :RHI.Texture;

export namespace Vortex {

struct RHIFramebufferDesc;

struct RHIFramebufferInfo {
	std::vector<RHIFormat> color_formats{};

	RHIFormat depth_format{RHIFormat::Unknown};
	uint32    sample_count{1};

	RHIFramebufferInfo() = default;
	RHIFramebufferInfo(const RHIFramebufferDesc& desc);

	bool operator==(const RHIFramebufferInfo&) const = default;

	RHIFramebufferInfo& addColorFormat(RHIFormat format)
	{
		color_formats.push_back(format);
		return *this;
	}

	RHIFramebufferInfo& setDepthFormat(RHIFormat format) noexcept
	{
		depth_format = format;
		return *this;
	}

	RHIFramebufferInfo& setSampleCount(uint32 new_sample_count) noexcept
	{
		sample_count = new_sample_count;
		return *this;
	}
};

struct RHIFramebufferAttachment {
	RHIRef<RHITextureView> texture_view{};

	RHILoadOp     load_op{RHILoadOp::Load};
	RHIStoreOp    store_op{RHIStoreOp::Store};
	RHIClearValue clear_value{};

	bool read_only{false};

	bool valid() const noexcept
	{
		return static_cast<bool>(texture_view);
	}

	RHIFramebufferAttachment& setTextureView(RHITextureView* new_texture_view) noexcept
	{
		texture_view = new_texture_view;
		return *this;
	}

	RHIFramebufferAttachment& setLoadOp(RHILoadOp new_load_op) noexcept
	{
		load_op = new_load_op;
		return *this;
	}

	RHIFramebufferAttachment& setStoreOp(RHIStoreOp new_store_op) noexcept
	{
		store_op = new_store_op;
		return *this;
	}

	RHIFramebufferAttachment& setClearValue(const RHIClearValue& new_clear_value) noexcept
	{
		clear_value = new_clear_value;
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

	std::vector<RHIFramebufferAttachment> color_attachments{};
	RHIFramebufferAttachment              depth_attachment{};

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

inline RHIFramebufferInfo::RHIFramebufferInfo(const RHIFramebufferDesc& desc) :
    sample_count(desc.sample_count)
{
	color_formats.reserve(desc.color_attachments.size());
	for (const auto& attachment : desc.color_attachments)
		color_formats.push_back(attachment.texture_view->getDesc().format);

	if (desc.depth_attachment.texture_view)
		depth_format = desc.depth_attachment.texture_view->getDesc().format;
}

class RHIFramebuffer : public RHIResource {
public:
	virtual const RHIFramebufferDesc& getDesc() const noexcept = 0;
	virtual const RHIFramebufferInfo& getFramebufferInfo() const noexcept = 0;
};

}        // namespace Vortex
