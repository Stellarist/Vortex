#pragma once

#include <string>
#include <vector>
#include <span>

#include "RHITypes.hpp"

class RHICommandList;
class RHIDevice;
class RHITexture;

// Resource
class RHIResource {
public:
	RHIResource() = default;
	virtual ~RHIResource() = default;

	RHIResource(const RHIResource&) = delete;
	RHIResource& operator=(const RHIResource&) = delete;

	RHIResource(RHIResource&&) noexcept = delete;
	RHIResource& operator=(RHIResource&&) noexcept = delete;
};


// Context
struct RHIContextDesc {
	RHIAPI    api{RHIAPI::Vulkan};
	RHIExtent extent{2560, 1440};
	RHIFormat format{RHIFormat::RGBA8_SRGB};
};

class RHIContext : public RHIResource {
public:
	virtual RHIDevice& getDevice() = 0;
	virtual RHIExtent  getExtent() const = 0;
	virtual RHIFormat  getFormat() const = 0;

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;

	virtual RHICommandList& getCommand() = 0;
	virtual RHITexture&     getBackbuffer() = 0;
};


// Buffer
struct RHIBufferDesc {
	uint64_t size{};
	uint32_t stride{};

	RHIFormat      format{RHIFormat::Unknown};
	RHIBufferUsage usage{RHIBufferUsage::None};
	RHIAccessMode  access{RHIAccessMode::None};

	RHIBufferDesc& setSize(uint64_t new_size)
	{
		size = new_size;
		return *this;
	}

	RHIBufferDesc& setStride(uint32_t new_stride)
	{
		stride = new_stride;
		return *this;
	}

	RHIBufferDesc& setUsage(RHIBufferUsage new_usage)
	{
		usage = new_usage;
		return *this;
	}

	RHIBufferDesc& setAccess(RHIAccessMode new_access)
	{
		access = new_access;
		return *this;
	}
};

class RHIBuffer : public RHIResource {
public:
	virtual const RHIBufferDesc& getDesc() const = 0;
};


// Texture
struct RHITextureDesc {
	uint32_t width{};
	uint32_t height{};
	uint32_t depth{1};

	uint32_t array_layers{1};
	uint32_t mip_levels{1};
	uint32_t sample_count{1};

	RHIFormat           format{RHIFormat::RGBA8_SRGB};
	RHITextureDimension dimension{RHITextureDimension::Texture2D};
	RHIColor            clear_color{};
	RHITextureUsage     usage{RHITextureUsage::None};

	RHITextureDesc& setWidth(uint32_t new_width)
	{
		width = new_width;
		return *this;
	}

	RHITextureDesc& setHeight(uint32_t new_height)
	{
		height = new_height;
		return *this;
	}

	RHITextureDesc& setDepth(uint32_t new_depth)
	{
		depth = new_depth;
		return *this;
	}

	RHITextureDesc& setArrayLayers(uint32_t new_array_layers)
	{
		array_layers = new_array_layers;
		return *this;
	}

	RHITextureDesc& setMipLevels(uint32_t new_mip_levels)
	{
		mip_levels = new_mip_levels;
		return *this;
	}

	RHITextureDesc& setSampleCount(uint32_t new_sample_count)
	{
		sample_count = new_sample_count;
		return *this;
	}

	RHITextureDesc& setFormat(RHIFormat new_format)
	{
		format = new_format;
		return *this;
	}

	RHITextureDesc& setDimension(RHITextureDimension new_dimension)
	{
		dimension = new_dimension;
		return *this;
	}

	RHITextureDesc& setClearColor(RHIColor new_clear_color)
	{
		clear_color = new_clear_color;
		return *this;
	}

	RHITextureDesc& setUsage(RHITextureUsage new_usage)
	{
		usage = new_usage;
		return *this;
	}
};

class RHITexture : public RHIResource {
public:
	virtual const RHITextureDesc& getDesc() const = 0;
};

class RHIStagingTexture : public RHIResource {
public:
	virtual const RHITextureDesc& getDesc() const = 0;
};


// Sampler
struct RHISamplerDesc {
	bool  mag_filter{true};
	bool  min_filter{true};
	bool  mip_filter{true};
	float max_anisotropy{1.0f};
	float mip_bias{};

	RHISamplerAddress address_u{RHISamplerAddress::Repeat};
	RHISamplerAddress address_v{RHISamplerAddress::Repeat};
	RHISamplerAddress address_w{RHISamplerAddress::Repeat};

	RHISamplerDesc& setMagFilter(bool new_mag_filter)
	{
		mag_filter = new_mag_filter;
		return *this;
	}

	RHISamplerDesc& setMinFilter(bool new_min_filter)
	{
		min_filter = new_min_filter;
		return *this;
	}

	RHISamplerDesc& setMipFilter(bool new_mip_filter)
	{
		mip_filter = new_mip_filter;
		return *this;
	}

	RHISamplerDesc& setAllFilters(bool filter)
	{
		min_filter = mag_filter = mip_filter = filter;
		return *this;
	}

	RHISamplerDesc& setMaxAnisotropy(float new_max_anisotropy)
	{
		max_anisotropy = new_max_anisotropy;
		return *this;
	}

	RHISamplerDesc& setMipBias(float new_mip_bias)
	{
		mip_bias = new_mip_bias;
		return *this;
	}

	RHISamplerDesc& setAddressU(RHISamplerAddress new_address_u)
	{
		address_u = new_address_u;
		return *this;
	}

	RHISamplerDesc& setAddressV(RHISamplerAddress new_address_v)
	{
		address_v = new_address_v;
		return *this;
	}

	RHISamplerDesc& setAddressW(RHISamplerAddress new_address_w)
	{
		address_w = new_address_w;
		return *this;
	}

	RHISamplerDesc& setAllAddressModes(RHISamplerAddress new_address)
	{
		address_u = address_v = address_w = new_address;
		return *this;
	}
};

class RHISampler : public RHIResource {
public:
	virtual const RHISamplerDesc& getDesc() const = 0;
};


// Shader
struct RHIShaderDesc {
	RHIShaderType type{RHIShaderType::None};

	std::string shader_name{};
	std::string entry_point{"main"};

	std::span<std::byte> codes{};

	RHIShaderDesc& setType(RHIShaderType new_type)
	{
		type = new_type;
		return *this;
	}

	RHIShaderDesc& setShaderName(std::string new_shader_name)
	{
		shader_name = std::move(new_shader_name);
		return *this;
	}

	RHIShaderDesc& setEntryPoint(std::string new_entry_point)
	{
		entry_point = std::move(new_entry_point);
		return *this;
	}

	RHIShaderDesc& setCodes(std::span<std::byte> new_codes)
	{
		codes = new_codes;
		return *this;
	}
};

class RHIShader : public RHIResource {
public:
	virtual const RHIShaderDesc& getDesc() const = 0;
};


// FrameBuffer
struct RHIFrameBufferAttachment {
	RHITexture* texture{};
	RHIFormat   format{RHIFormat::RGBA32_FLOAT};
	bool        read_only{false};

	bool valid() const
	{
		return texture;
	}

	RHIFrameBufferAttachment& setTexture(RHITexture* new_texture)
	{
		texture = new_texture;
		return *this;
	}

	RHIFrameBufferAttachment& setFormat(RHIFormat new_format)
	{
		format = new_format;
		return *this;
	}

	RHIFrameBufferAttachment& setReadOnly(bool new_read_only)
	{
		read_only = new_read_only;
		return *this;
	}
};

struct RHIFrameBufferDesc {
	uint32_t width{0};
	uint32_t height{0};
	uint32_t array_size{1};
	uint32_t sample_count{1};
	uint32_t sample_quality{0};

	std::vector<RHIFrameBufferAttachment> color_attachments{};
	RHIFrameBufferAttachment              depth_attachment{};

	std::vector<RHIFormat> color_formats{};
	RHIFormat              depth_format{RHIFormat::Unknown};

	RHIFrameBufferDesc& setWidth(uint32_t new_width)
	{
		width = new_width;
		return *this;
	}

	RHIFrameBufferDesc& setHeight(uint32_t new_height)
	{
		height = new_height;
		return *this;
	}

	RHIFrameBufferDesc& setArraySize(uint32_t new_array_size)
	{
		array_size = new_array_size;
		return *this;
	}

	RHIFrameBufferDesc& setSampleCount(uint32_t new_sample_count)
	{
		sample_count = new_sample_count;
		return *this;
	}

	RHIFrameBufferDesc& setSampleQuality(uint32_t new_sample_quality)
	{
		sample_quality = new_sample_quality;
		return *this;
	}

	RHIFrameBufferDesc& addColorAttachment(const RHIFrameBufferAttachment& attachment)
	{
		color_attachments.push_back(attachment);
		return *this;
	}

	RHIFrameBufferDesc& addColorAttachment(RHITexture* texture)
	{
		color_attachments.push_back(RHIFrameBufferAttachment().setTexture(texture));
		return *this;
	}

	RHIFrameBufferDesc& setDepthAttachment(const RHIFrameBufferAttachment& attachment)
	{
		depth_attachment = attachment;
		return *this;
	}

	RHIFrameBufferDesc& setDepthAttachment(RHITexture* texture)
	{
		depth_attachment = RHIFrameBufferAttachment().setTexture(texture);
		return *this;
	}
};

class RHIFrameBuffer : public RHIResource {
public:
	virtual const RHIFrameBufferDesc& getDesc() const = 0;
};
