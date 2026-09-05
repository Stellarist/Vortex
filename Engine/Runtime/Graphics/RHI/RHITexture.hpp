export module Runtime.Graphics:RHI.Texture;

import Core;
import :RHI.Resource;
import :RHI.Types;

export namespace Vortex {

struct RHITextureDesc {
	uint32 width{};
	uint32 height{};
	uint32 depth{1};

	uint32 array_layers{1};
	uint32 mip_levels{1};
	uint32 sample_count{1};

	RHIFormat format{RHIFormat::RGBA8_SRGB};
	RHITextureDimension dimension{RHITextureDimension::Texture2D};
	RHIColor clear_color{};
	RHITextureUsage usage{RHITextureUsage::None};

	bool operator==(const RHITextureDesc&) const noexcept = default;

	RHITextureDesc& setWidth(uint32 new_width) noexcept
	{
		width = new_width;
		return *this;
	}

	RHITextureDesc& setHeight(uint32 new_height) noexcept
	{
		height = new_height;
		return *this;
	}

	RHITextureDesc& setDepth(uint32 new_depth) noexcept
	{
		depth = new_depth;
		return *this;
	}

	RHITextureDesc& setArrayLayers(uint32 new_array_layers) noexcept
	{
		array_layers = new_array_layers;
		return *this;
	}

	RHITextureDesc& setMipLevels(uint32 new_mip_levels) noexcept
	{
		mip_levels = new_mip_levels;
		return *this;
	}

	RHITextureDesc& setSampleCount(uint32 new_sample_count) noexcept
	{
		sample_count = new_sample_count;
		return *this;
	}

	RHITextureDesc& setFormat(RHIFormat new_format) noexcept
	{
		format = new_format;
		return *this;
	}

	RHITextureDesc& setDimension(RHITextureDimension new_dimension) noexcept
	{
		dimension = new_dimension;
		return *this;
	}

	RHITextureDesc& setClearColor(RHIColor new_clear_color) noexcept
	{
		clear_color = new_clear_color;
		return *this;
	}

	RHITextureDesc& setUsage(RHITextureUsage new_usage) noexcept
	{
		usage = new_usage;
		return *this;
	}
};

class RHITexture : public RHIResource {
public:
	virtual const RHITextureDesc& getDesc() const noexcept = 0;
	virtual RHIResourceState getState() const noexcept = 0;
};

class RHIStagingTexture : public RHIResource {
public:
	virtual const RHITextureDesc& getDesc() const noexcept = 0;
};


struct RHITextureViewDesc {
	RHIRef<RHITexture> texture{};
	RHITextureViewType type{RHITextureViewType::ShaderResource};
	RHITextureViewDimension dimension{RHITextureViewDimension::Automatic};
	RHIFormat format{RHIFormat::Unknown};
	RHITextureSubresource subresource{};

	RHITextureViewDesc& setTexture(RHITexture* new_texture) noexcept
	{
		texture = new_texture;
		return *this;
	}

	RHITextureViewDesc& setType(RHITextureViewType new_type) noexcept
	{
		type = new_type;
		return *this;
	}

	RHITextureViewDesc& setDimension(RHITextureViewDimension new_dimension) noexcept
	{
		dimension = new_dimension;
		return *this;
	}

	RHITextureViewDesc& setFormat(RHIFormat new_format) noexcept
	{
		format = new_format;
		return *this;
	}

	RHITextureViewDesc& setSubresource(const RHITextureSubresource& new_subresource) noexcept
	{
		subresource = new_subresource;
		return *this;
	}
};

class RHITextureView : public RHIResource {
public:
	virtual const RHITextureViewDesc& getDesc() const noexcept = 0;
	virtual RHITexture& getTexture() const noexcept = 0;
};


struct RHISamplerDesc {
	bool mag_filter{true};
	bool min_filter{true};
	bool mip_filter{true};
	float mip_bias{};

	RHISamplerAddress address_u{RHISamplerAddress::Repeat};
	RHISamplerAddress address_v{RHISamplerAddress::Repeat};
	RHISamplerAddress address_w{RHISamplerAddress::Repeat};

	RHISamplerDesc& setMagFilter(bool new_mag_filter) noexcept
	{
		mag_filter = new_mag_filter;
		return *this;
	}

	RHISamplerDesc& setMinFilter(bool new_min_filter) noexcept
	{
		min_filter = new_min_filter;
		return *this;
	}

	RHISamplerDesc& setMipFilter(bool new_mip_filter) noexcept
	{
		mip_filter = new_mip_filter;
		return *this;
	}

	RHISamplerDesc& setAllFilters(bool filter) noexcept
	{
		min_filter = mag_filter = mip_filter = filter;
		return *this;
	}

	RHISamplerDesc& setMipBias(float new_mip_bias) noexcept
	{
		mip_bias = new_mip_bias;
		return *this;
	}

	RHISamplerDesc& setAddressU(RHISamplerAddress new_address_u) noexcept
	{
		address_u = new_address_u;
		return *this;
	}

	RHISamplerDesc& setAddressV(RHISamplerAddress new_address_v) noexcept
	{
		address_v = new_address_v;
		return *this;
	}

	RHISamplerDesc& setAddressW(RHISamplerAddress new_address_w) noexcept
	{
		address_w = new_address_w;
		return *this;
	}

	RHISamplerDesc& setAllAddressModes(RHISamplerAddress new_address) noexcept
	{
		address_u = address_v = address_w = new_address;
		return *this;
	}
};

class RHISampler : public RHIResource {
public:
	virtual const RHISamplerDesc& getDesc() const noexcept = 0;
};

}        // namespace Vortex
