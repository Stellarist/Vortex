export module Runtime.Graphics:RHI.Sampler;

import :RHI.Resource;
import :RHI.Types;

export namespace Vortex {

struct RHISamplerDesc {
	bool  mag_filter{true};
	bool  min_filter{true};
	bool  mip_filter{true};
	float max_anisotropy{1.0f};
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

	RHISamplerDesc& setMaxAnisotropy(float new_max_anisotropy) noexcept
	{
		max_anisotropy = new_max_anisotropy;
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
